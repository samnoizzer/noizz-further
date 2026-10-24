#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Custom Neon Green & Gunmetal Gray LookAndFeel
class NoizzLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NoizzLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff00ff66));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ff66));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1a1d21));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff00ff66));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0d0f12));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2e343b));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, const float rotaryStartAngle,
                           const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 8.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // 3D Gunmetal Knob Body
        g.setColour(juce::Colour(0xff1e2229));
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(juce::Colour(0xff3a404a));
        g.drawEllipse(rx, ry, rw, rw, 2.0f);

        // Outer Track
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius + 4.0f, radius + 4.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff14171a));
        g.strokePath(bgArc, juce::PathStrokeType(3.5f));

        // Neon Green Value Arc
        juce::Path valArc;
        valArc.addCentredArc(centreX, centreY, radius + 4.0f, radius + 4.0f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xff00ff66));
        g.strokePath(valArc, juce::PathStrokeType(3.5f));

        // Pointer Notch
        juce::Path p;
        auto pointerLength = radius * 0.65f;
        p.addRectangle (-1.5f, -radius, 3.0f, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour(0xff00ff66));
        g.fillPath (p);
    }
};

class NoizzFurtherAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    NoizzFurtherAudioProcessorEditor (NoizzFurtherAudioProcessor&);
    ~NoizzFurtherAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateBpmFromStep(float delta);

    NoizzFurtherAudioProcessor& audioProcessor;
    NoizzLookAndFeel customLookAndFeel;

    // Knobs & Buttons
    juce::Slider widthSlider, lowBypassSlider, lfoRateSlider, driveSlider;
    juce::ToggleButton lfoSyncButton { "SYNC" };
    juce::ToggleButton monoCheckButton { "MONO CHECK" };

    // Numerical LED BPM Monitor with Up/Down Switches & Click-to-Edit
    juce::Label bpmLedLabel;
    juce::TextButton bpmUpButton { "+" }, bpmDownButton { "-" };

    // APVTS Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> widthAttach, lowBypassAttach, lfoRateAttach, driveAttach;
    std::unique_ptr<ButtonAttachment> lfoSyncAttach, monoCheckAttach;

    // Stereoscope Display Buffer
    std::vector<NoizzFurtherAudioProcessor::StereoSample> visualPoints;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoizzFurtherAudioProcessorEditor)
};
