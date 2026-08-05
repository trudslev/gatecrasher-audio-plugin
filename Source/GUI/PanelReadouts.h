#pragma once

#include "../PluginProcessor.h"
#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

// The numeric value under each knob ("-18.5 dB", "165 ms", "0.72"...) and the algorithm selector's
// four corner labels, whose lit one follows the current Algorithm choice.
//
// These are the panel's remaining state-dependent text. They were previously baked into the
// background render, which meant they showed one frozen set of values from whenever that render was
// captured and never responded to anything - turning a knob moved the pointer but not the number
// under it. PanelChrome draws the static engraved layer around them; this draws only what changes.
//
// Values come from each parameter's own getText(), so units and decimal places match what the host
// shows for the same parameter and there is no second formatting convention to keep in sync. The
// text is cached and compared per knob, so the poll only repaints the specific readouts whose
// digits actually changed rather than the whole layer.
//
// Polled rather than driven by parameter listeners: this is display-only, a listener per parameter
// would fire on the audio thread, and the same 30Hz tick already covers the algorithm labels.
class PanelReadouts final : public juce::Component, private juce::Timer
{
public:
    explicit PanelReadouts(GatecrasherAudioProcessor& processor);
    ~PanelReadouts() override;

    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;
    juce::String currentValueText(size_t knobIndex) const;
    int currentAlgorithmIndex() const;

    GatecrasherAudioProcessor& processorRef;

    // Parallel to GatecrasherTheme::Layout::knobs; entries for knobs with no readout stay empty.
    std::array<juce::String, 15> displayedValues;
    int displayedAlgorithm = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelReadouts)
};
