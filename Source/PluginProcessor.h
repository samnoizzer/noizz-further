#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class NoizzFurtherAudioProcessor : public juce::AudioProcessor
{
public:
    NoizzFurtherAudioProcessor();
    ~NoizzFurtherAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Thread-safe lock-free buffer for the real-time Stereoscope UI
    struct StereoSample { float l; float r; };
    void pushVisualizerSamples(const float* l, const float* r, int numSamples);
    void pullVisualizerSamples(std::vector<StereoSample>& dest);

private:
    // Crossover Filter for Low Bypass
    juce::dsp::LinkwitzRileyFilter<float> lowPassFilter;
    juce::dsp::LinkwitzRileyFilter<float> highPassFilter;

    // Complementary Comb Delay Lines & All-Pass Cascade for Pseudo-Stereo
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> combDelayLine { 4800 };
    std::array<juce::dsp::IIR::Filter<float>, 4> allpassCascade;

    double currentSampleRate = 44100.0;
    float lfoPhase = 0.0f;

    // Stereoscope Ring Buffer
    static constexpr int scopeFifoSize = 4096;
    juce::AbstractFifo scopeFifo { scopeFifoSize };
    std::array<StereoSample, scopeFifoSize> scopeBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoizzFurtherAudioProcessor)
};
