#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AirConditionerAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AirConditionerAudioProcessorEditor (AirConditionerAudioProcessor&);
    ~AirConditionerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override { repaint(); }
    int  getTemp() const;
    int  getMode() const;
    void setParam (const juce::String& id, float realValue);

    AirConditionerAudioProcessor& proc;
    juce::Rectangle<float> display { 330, 60, 130, 76 },
                           upBtn   { 472, 60, 48, 34 },
                           downBtn { 472, 102, 48, 34 },
                           sw      { 330, 176, 190, 40 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirConditionerAudioProcessorEditor)
};
