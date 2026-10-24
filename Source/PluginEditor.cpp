#include "PluginProcessor.h"
#include "PluginEditor.h"

NoizzFurtherAudioProcessorEditor::NoizzFurtherAudioProcessorEditor (NoizzFurtherAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    setSize (860, 520);

    auto setupKnob = [this](juce::Slider& slider, const juce::String& suffix)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
        slider.setTextValueSuffix(suffix);
        addAndMakeVisible(slider);
    };

    setupKnob(widthSlider, " %");
    setupKnob(lowBypassSlider, " Hz");
    setupKnob(lfoRateSlider, "");
    setupKnob(driveSlider, "");

    addAndMakeVisible(lfoSyncButton);
    addAndMakeVisible(monoCheckButton);

    lfoSyncButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xff00ff66));
    lfoSyncButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00ff66));
    monoCheckButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xff00ff66));
    monoCheckButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00ff66));

    // Setup Numerical BPM LED Monitor (Click to enter manually, or use +/- switches)
    bpmLedLabel.setEditable(true, true, false);
    bpmLedLabel.setJustificationType(juce::Justification::centred);
    bpmLedLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 18.0f, juce::Font::bold));
    bpmLedLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff070a08));
    bpmLedLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff66));
    bpmLedLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff00ff66));
    bpmLedLabel.onTextChange = [this]()
    {
        float val = bpmLedLabel.getText().getFloatValue();
        val = juce::jlimit(20.0f, 300.0f, val);
        if (auto* param = audioProcessor.apvts.getParameter("BPM"))
            param->setValueNotifyingHost(param->convertTo0to1(val));
        bpmLedLabel.setText(juce::String(val, 1) + " BPM", juce::dontSendNotification);
    };
    addAndMakeVisible(bpmLedLabel);

    bpmUpButton.onClick   = [this]() { updateBpmFromStep(1.0f); };
    bpmDownButton.onClick = [this]() { updateBpmFromStep(-1.0f); };
    addAndMakeVisible(bpmUpButton);
    addAndMakeVisible(bpmDownButton);

    // Connect Knobs & Switches to APVTS
    widthAttach     = std::make_unique<SliderAttachment>(audioProcessor.apvts, "WIDTH", widthSlider);
    lowBypassAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "LOW_BYPASS", lowBypassSlider);
    lfoRateAttach   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "LFO_RATE", lfoRateSlider);
    driveAttach     = std::make_unique<SliderAttachment>(audioProcessor.apvts, "DRIVE", driveSlider);
    lfoSyncAttach   = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "LFO_SYNC", lfoSyncButton);
    monoCheckAttach = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "MONO_CHECK", monoCheckButton);

    float initBpm = audioProcessor.apvts.getRawParameterValue("BPM")->load();
    bpmLedLabel.setText(juce::String(initBpm, 1) + " BPM", juce::dontSendNotification);

    startTimerHz(60); // 60 FPS Stereoscope refresh rate
}

NoizzFurtherAudioProcessorEditor::~NoizzFurtherAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void NoizzFurtherAudioProcessorEditor::updateBpmFromStep(float delta)
{
    float current = audioProcessor.apvts.getRawParameterValue("BPM")->load();
    float next = juce::jlimit(20.0f, 300.0f, current + delta);
    if (auto* param = audioProcessor.apvts.getParameter("BPM"))
        param->setValueNotifyingHost(param->convertTo0to1(next));
    bpmLedLabel.setText(juce::String(next, 1) + " BPM", juce::dontSendNotification);
}

void NoizzFurtherAudioProcessorEditor::timerCallback()
{
    audioProcessor.pullVisualizerSamples(visualPoints);
    if (visualPoints.size() > 1024)
        visualPoints.erase(visualPoints.begin(), visualPoints.end() - 1024);

    if (!bpmLedLabel.isBeingEdited())
    {
        float currentBpm = audioProcessor.apvts.getRawParameterValue("BPM")->load();
        bpmLedLabel.setText(juce::String(currentBpm, 1) + " BPM", juce::dontSendNotification);
    }

    repaint();
}

void NoizzFurtherAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1. Gunmetal Gray Outer Rack Chassis
    g.fillAll (juce::Colour(0xff26292e));

    // Inner Black Panel
    juce::Rectangle<int> centerPanel(150, 20, 560, 480);
    g.setColour(juce::Colour(0xff0d0f12));
    g.fillRoundedRectangle(centerPanel.toFloat(), 8.0f);
    g.setColour(juce::Colour(0xff3d434d));
    g.drawRoundedRectangle(centerPanel.toFloat(), 8.0f, 2.0f);

    // 2. Futuristic PCB Lining Traces
    g.setColour(juce::Colour(0x4400ff66));
    for (int i = 0; i < 6; ++i)
    {
        float yOffset = 45.0f + i * 28.0f;
        g.drawLine(170.0f, yOffset, 240.0f, yOffset, 1.5f);
        g.drawLine(240.0f, yOffset, 260.0f, yOffset + 15.0f, 1.5f);
        g.fillEllipse(258.0f, yOffset + 13.0f, 5.0f, 5.0f);

        g.drawLine(690.0f, yOffset, 620.0f, yOffset, 1.5f);
        g.drawLine(620.0f, yOffset, 600.0f, yOffset + 15.0f, 1.5f);
        g.fillEllipse(597.0f, yOffset + 13.0f, 5.0f, 5.0f);
    }

    // 3. Center Logo & Title Header
    g.setColour(juce::Colour(0xff00ff66));
    for (int r = 0; r < 4; ++r)
        g.drawEllipse(430.0f - (45.0f + r * 6.0f), 85.0f - (45.0f + r * 6.0f), (45.0f + r * 6.0f) * 2.0f, (45.0f + r * 6.0f) * 2.0f, 1.2f);

    juce::Path pyramid;
    pyramid.addTriangle(430.0f, 45.0f, 390.0f, 115.0f, 470.0f, 115.0f);
    g.strokePath(pyramid, juce::PathStrokeType(1.8f));

    g.setFont(juce::Font(28.0f, juce::Font::bold));
    g.drawText("NOIZZ STUDIO", 280, 125, 300, 35, juce::Justification::centred);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawText("NOIZZ FURTHER", 250, 175, 360, 40, juce::Justification::centred);

    g.setColour(juce::Colours::silver);
    g.setFont(juce::Font(12.0f, juce::Font::plain));
    g.drawText("STEREO SPREAD // SPATIAL PROCESSOR", 250, 212, 360, 20, juce::Justification::centred);

    // 4. Stereoscope Monitor Screen
    juce::Rectangle<int> scopeRect(230, 250, 400, 225);
    g.setColour(juce::Colour(0xff050806));
    g.fillRoundedRectangle(scopeRect.toFloat(), 6.0f);
    g.setColour(juce::Colour(0xff00ff66).withAlpha(0.6f));
    g.drawRoundedRectangle(scopeRect.toFloat(), 6.0f, 1.5f);

    // Scope Grid Lines
    g.setColour(juce::Colour(0x2200ff66));
    g.drawVerticalLine(scopeRect.getCentreX(), (float)scopeRect.getY(), (float)scopeRect.getBottom());
    g.drawHorizontalLine(scopeRect.getCentreY(), (float)scopeRect.getX(), (float)scopeRect.getRight());
    g.drawLine((float)scopeRect.getX(), (float)scopeRect.getY(), (float)scopeRect.getRight(), (float)scopeRect.getBottom(), 1.0f);
    g.drawLine((float)scopeRect.getX(), (float)scopeRect.getBottom(), (float)scopeRect.getRight(), (float)scopeRect.getY(), 1.0f);

    // Plot Real-Time Lissajous Polar Vectorscope
    if (!visualPoints.empty())
    {
        g.setColour(juce::Colour(0xff00ff66));
        float cx = (float)scopeRect.getCentreX();
        float cy = (float)scopeRect.getCentreY();
        float scale = 95.0f;

        for (size_t i = 1; i < visualPoints.size(); ++i)
        {
            float mid1  = (visualPoints[i - 1].l + visualPoints[i - 1].r) * 0.5f;
            float side1 = (visualPoints[i - 1].l - visualPoints[i - 1].r) * 0.5f;
            float mid2  = (visualPoints[i].l + visualPoints[i].r) * 0.5f;
            float side2 = (visualPoints[i].l - visualPoints[i].r) * 0.5f;

            float x1 = juce::jlimit((float)scopeRect.getX(), (float)scopeRect.getRight(),  cx + side1 * scale);
            float y1 = juce::jlimit((float)scopeRect.getY(), (float)scopeRect.getBottom(), cy - mid1  * scale);
            float x2 = juce::jlimit((float)scopeRect.getX(), (float)scopeRect.getRight(),  cx + side2 * scale);
            float y2 = juce::jlimit((float)scopeRect.getY(), (float)scopeRect.getBottom(), cy - mid2  * scale);

            g.drawLine(x1, y1, x2, y2, 1.2f);
        }
    }

    // 5. Knob Labels
    g.setColour(juce::Colour(0xff00ff66));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("WIDTH (0-200%)", 20, 35, 115, 20, juce::Justification::centred);
    g.drawText("LOW BYPASS",     20, 195, 115, 20, juce::Justification::centred);
    g.drawText("SIDE DRIVE",     725, 35, 115, 20, juce::Justification::centred);
    g.drawText("LFO RATE",       725, 195, 115, 20, juce::Justification::centred);
}

void NoizzFurtherAudioProcessorEditor::resized()
{
    // Left Column Controls
    widthSlider.setBounds(22, 60, 110, 115);
    lowBypassSlider.setBounds(22, 220, 110, 115);
    monoCheckButton.setBounds(25, 365, 110, 30);

    // Right Column Controls
    driveSlider.setBounds(728, 60, 110, 115);
    lfoRateSlider.setBounds(728, 220, 110, 115);
    lfoSyncButton.setBounds(740, 350, 90, 26);

    // Numerical LED BPM Monitor & +/- Switches
    bpmLedLabel.setBounds(722, 385, 92, 32);
    bpmUpButton.setBounds(816, 385, 24, 15);
    bpmDownButton.setBounds(816, 402, 24, 15);
}
