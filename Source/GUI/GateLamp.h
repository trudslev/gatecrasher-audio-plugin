#pragma once

#include "../PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// The GATE OPEN lamp + its label (GATECRASHER-GUI-SPEC.md section 5) - the plugin's one dedicated
// live-state LED per BRAND.md's component grammar. Polls processor.isGateOpen() on a ~60fps Timer
// rather than reading it once, since it's not an APVTS parameter.
//
// Opening must be instantaneous - "no fade-in, no smoothing" is section 5's most emphatic
// requirement (also "10. What matters most" #2) - so the lit state snaps to full brightness on the
// exact poll it turns true, never eased in over several frames. Closing is allowed a short cosmetic
// glow-decay (~40ms) added at this GUI layer only: GateEnvelopeGenerator.h's own comment notes the
// DSP deliberately leaves this to the GUI, decoupling the audible release curve from how the lamp
// looks as it goes dark.
class GateLamp final : public juce::Component, private juce::Timer
{
public:
    explicit GateLamp(GatecrasherAudioProcessor& processor);
    ~GateLamp() override;

    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;

    GatecrasherAudioProcessor& processorRef;

    // 0..1 - snaps to 1 the instant isGateOpen() reads true, decays exponentially toward 0 once it
    // reads false again. Drives the lamp's glow/fill only; the GATE OPEN label switches on the raw
    // isGateOpen() boolean directly (spec doesn't ascribe any decay to the label itself).
    float displayedGlow = 0.0f;
};
