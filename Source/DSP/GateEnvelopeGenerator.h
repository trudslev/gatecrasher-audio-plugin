#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>

enum class GateShape { hard, soft };

// Threshold/Attack/Hold/Release state machine driven by TriggerDetector's output. Produces both a
// continuous gain envelope (applied to the wet reverb signal, and polled for the oscilloscope
// trace) and an instantaneous gateOpen flag that flips on the exact sample the gate opens/closes,
// decoupled from the envelope's curve shape - this is what lets the GUI lamp fire with no fade-in.
// Shape governs the release-phase curve only; attack and hold are unaffected by it.
class GateEnvelopeGenerator
{
public:
    void prepare(double newSampleRate);
    void reset();

    // Consumes numSamples of trigger level (see TriggerDetector), writes the gain envelope
    // (0..1) into outGain.
    void process(const float* triggerLevel, int numSamples,
                 float thresholdDb, float attackMs, float holdMs, float releaseMs,
                 GateShape shape, float* outGain);

    bool isGateOpen() const noexcept { return gateOpen.load(std::memory_order_relaxed); }
    float getCurrentEnvelope() const noexcept { return currentGain.load(std::memory_order_relaxed); }

private:
    enum class State { closed, attack, hold, release };

    double sampleRate = 44100.0;
    State state = State::closed;
    float gain = 0.0f;
    double holdCounterSamples = 0.0;
    double releaseCounterSamples = 0.0;
    float releaseStartGain = 0.0f;

    std::atomic<bool> gateOpen { false };
    std::atomic<float> currentGain { 0.0f };
};
