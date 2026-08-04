#pragma once

#include "../PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

// The gate envelope oscilloscope - the product's centrepiece (GATECRASHER-GUI-SPEC.md section 5,
// "10. What matters most" #1). Fully code-drawn: polls processor.getGateEnvelope()/getTriggerLevel()
// on a ~60fps Timer and accumulates its own local scrolling-history ring buffer, since
// PluginProcessor deliberately doesn't maintain a shared one (see the comment on those getters in
// PluginProcessor.h). Because the polled envelope value already reflects GateEnvelopeGenerator's
// Shape-driven release curve, this never needs to special-case Shape (Hard/Soft) itself - the trace
// just draws whatever the DSP is actually doing.
class GateScope final : public juce::Component, private juce::Timer
{
public:
    explicit GateScope(GatecrasherAudioProcessor& processor);
    ~GateScope() override;

    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;

    GatecrasherAudioProcessor& processorRef;

    // Comfortably more than the scope's visible column count (~172 at 2px/frame across a 344px-
    // wide rect) so the ring buffer never runs out of look-back history.
    static constexpr int historySize = 200;
    std::array<float, historySize> envelopeHistory{};
    std::array<float, historySize> triggerDisplayHistory{};
    int writeIndex = 0;

    // Peak-hold-with-exponential-decay for the grey input-waveform underlay (section 5: "1px
    // vertical strokes... exponential decay from each trigger") - getTriggerLevel() itself is an
    // instantaneous sample, this is the extra local smoothing that makes it read as a decaying bar.
    float triggerDisplayDecayed = 0.0f;

    // Scrolls the 44px-pitch vertical grid in lockstep with the 2px/frame trace scroll.
    float gridScrollPhase = 0.0f;
};
