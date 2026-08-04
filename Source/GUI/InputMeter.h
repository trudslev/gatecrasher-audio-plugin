#pragma once

#include "../PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Segmented input/trigger-source LED bar (GATECRASHER-GUI-SPEC.md section 7), reading
// processor.getInputMeterLevel() - already ballistics-shaped in PluginProcessor (fast attack, slow
// ~0.12 release coefficient) so this just maps the live level to lit segments each repaint, no
// further smoothing needed here. Also draws the threshold marker, read live from the Threshold
// APVTS parameter so it always shows where the gate will actually trigger.
class InputMeter final : public juce::Component, private juce::Timer
{
public:
    explicit InputMeter(GatecrasherAudioProcessor& processor);
    ~InputMeter() override;

    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;

    GatecrasherAudioProcessor& processorRef;
};
