#include "PluginProcessor.h"
#include "PluginEditor.h"

NoizzFurtherAudioProcessor::NoizzFurtherAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

NoizzFurtherAudioProcessor::~NoizzFurtherAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout NoizzFurtherAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"WIDTH", 1}, "Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f));

    juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 1.0f, 0.25f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"LOW_BYPASS", 1}, "Low Bypass", freqRange, 20.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"LFO_SYNC", 1}, "LFO Sync", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"BPM", 1}, "BPM",
        juce::NormalisableRange<float>(20.0f, 300.0f, 0.1f), 120.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"LFO_RATE", 1}, "LFO Rate",
        juce::NormalisableRange<float>(0.0f, 11.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"DRIVE", 1}, "Drive",
        juce::NormalisableRange<float>(0.0f, 11.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"MONO_CHECK", 1}, "Mono Check", false));

    return { params.begin(), params.end() };
}

const juce::String NoizzFurtherAudioProcessor::getName() const { return JucePlugin_Name; }
bool NoizzFurtherAudioProcessor::acceptsMidi() const { return false; }
bool NoizzFurtherAudioProcessor::producesMidi() const { return false; }
bool NoizzFurtherAudioProcessor::isMidiEffect() const { return false; }
double NoizzFurtherAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int NoizzFurtherAudioProcessor::getNumPrograms() { return 1; }
int NoizzFurtherAudioProcessor::getCurrentProgram() { return 0; }
void NoizzFurtherAudioProcessor::setCurrentProgram (int) {}
const juce::String NoizzFurtherAudioProcessor::getProgramName (int) { return {}; }
void NoizzFurtherAudioProcessor::changeProgramName (int, const juce::String&) {}

void NoizzFurtherAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec stereoSpec;
    stereoSpec.sampleRate = sampleRate;
    stereoSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    stereoSpec.numChannels = 2;

    lowPassFilter.prepare(stereoSpec);
    lowPassFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);

    highPassFilter.prepare(stereoSpec);
    highPassFilter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    juce::dsp::ProcessSpec monoSpec = stereoSpec;
    monoSpec.numChannels = 1;

    combDelayLine.prepare(monoSpec);
    combDelayLine.setMaximumDelayInSamples(static_cast<int>(sampleRate * 0.05) + 64);

    const float apFreqs[4] = { 220.0f, 580.0f, 1450.0f, 3700.0f };
    for (int i = 0; i < 4; ++i)
    {
        allpassCascade[i].prepare(monoSpec);
        allpassCascade[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, apFreqs[i], 0.707f);
    }

    lfoPhase = 0.0f;
}

void NoizzFurtherAudioProcessor::releaseResources() {}

bool NoizzFurtherAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo() &&
        layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    return true;
}

void NoizzFurtherAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float widthParam  = apvts.getRawParameterValue("WIDTH")->load();
    float lowBypassHz = apvts.getRawParameterValue("LOW_BYPASS")->load();
    bool  lfoSync     = apvts.getRawParameterValue("LFO_SYNC")->load() > 0.5f;
    float manualBpm   = apvts.getRawParameterValue("BPM")->load();
    float lfoRateVal  = apvts.getRawParameterValue("LFO_RATE")->load();
    float driveVal    = apvts.getRawParameterValue("DRIVE")->load();
    bool  monoCheck   = apvts.getRawParameterValue("MONO_CHECK")->load() > 0.5f;

    lowPassFilter.setCutoffFrequency(lowBypassHz);
    highPassFilter.setCutoffFrequency(lowBypassHz);

    double activeBpm = static_cast<double>(manualBpm);
    if (lfoSync)
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto pos = playHead->getPosition())
            {
                if (pos->getBpm().hasValue())
                    activeBpm = *pos->getBpm();
            }
        }
    }

    float lfoFreqHz = 0.0f;
    if (lfoRateVal > 0.01f)
    {
        if (lfoSync)
        {
            const float beatMultipliers[8] = { 0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 8.0f };
            int idx = juce::jlimit(0, 7, static_cast<int>((lfoRateVal / 11.0f) * 7.99f));
            lfoFreqHz = static_cast<float>((activeBpm / 60.0) * beatMultipliers[idx]);
        }
        else
        {
            lfoFreqHz = (lfoRateVal / 11.0f) * 15.0f;
        }
    }

    const int numSamples = buffer.getNumSamples();
    auto* leftChannel  = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(totalNumOutputChannels > 1 ? 1 : 0);

    const float delaySamplesA = static_cast<float>(0.0033 * currentSampleRate);
    const float delaySamplesB = static_cast<float>(0.0079 * currentSampleRate);
    const float baseWidthGain = widthParam / 100.0f;
    const float driveGain     = 1.0f + (driveVal * 0.9f);

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = leftChannel[s];
        float inR = rightChannel[s];

        float lowL  = lowPassFilter.processSample(0, inL);
        float lowR  = lowPassFilter.processSample(1, inR);
        float highL = highPassFilter.processSample(0, inL);
        float highR = highPassFilter.processSample(1, inR);

        float midHigh  = 0.5f * (highL + highR);
        float origSide = 0.5f * (highL - highR);

        float apOut = midHigh;
        for (auto& ap : allpassCascade)
            apOut = ap.processSample(apOut);

        combDelayLine.pushSample(0, apOut);
        float tapA = combDelayLine.popSample(0, delaySamplesA, false);
        float tapB = combDelayLine.popSample(0, delaySamplesB, true);
        float pseudoSide = 0.7f * (apOut - 0.6f * tapA + 0.4f * tapB);

        float sideSignal = origSide + pseudoSide;

        if (driveVal > 0.001f)
            sideSignal = std::tanh(sideSignal * driveGain) / std::tanh(driveGain);

        float lfoMod = 1.0f;
        if (lfoRateVal > 0.01f)
        {
            lfoPhase += static_cast<float>(lfoFreqHz / currentSampleRate);
            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;

            float sineWave = 0.5f * (1.0f + std::sin(juce::MathConstants<float>::twoPi * lfoPhase));
            lfoMod = 0.25f + 0.75f * sineWave;
        }

        float finalSide = sideSignal * baseWidthGain * lfoMod;

        float outHighL = midHigh + finalSide;
        float outHighR = midHigh - finalSide;

        float finalL = lowL + outHighL;
        float finalR = lowR + outHighR;

        if (monoCheck)
        {
            float monoSum = 0.5f * (finalL + finalR);
            finalL = monoSum;
            finalR = monoSum;
        }

        leftChannel[s]  = finalL;
        rightChannel[s] = finalR;
    }

    pushVisualizerSamples(leftChannel, rightChannel, numSamples);
}

void NoizzFurtherAudioProcessor::pushVisualizerSamples(const float* l, const float* r, int numSamples)
{
    int start1, size1, start2, size2;
    scopeFifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        scopeBuffer[start1 + i] = { l[i], r[i] };
    for (int i = 0; i < size2; ++i)
        scopeBuffer[start2 + i] = { l[size1 + i], r[size1 + i] };

    scopeFifo.finishedWrite(size1 + size2);
}

void NoizzFurtherAudioProcessor::pullVisualizerSamples(std::vector<StereoSample>& dest)
{
    int numReady = scopeFifo.getNumReady();
    if (numReady <= 0) return;

    int start1, size1, start2, size2;
    scopeFifo.prepareToRead(numReady, start1, size1, start2, size2);

    dest.reserve(dest.size() + size1 + size2);
    for (int i = 0; i < size1; ++i)
        dest.push_back(scopeBuffer[start1 + i]);
    for (int i = 0; i < size2; ++i)
        dest.push_back(scopeBuffer[start2 + i]);

    scopeFifo.finishedRead(size1 + size2);
}

bool NoizzFurtherAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* NoizzFurtherAudioProcessor::createEditor()
{
    return new NoizzFurtherAudioProcessorEditor (*this);
}

void NoizzFurtherAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NoizzFurtherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoizzFurtherAudioProcessor();
}
