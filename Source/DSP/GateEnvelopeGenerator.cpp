#include "GateEnvelopeGenerator.h"
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

namespace
{
    // SOFT's pow(1 - t, 2.2) release curve is specced to visually read as roughly 5x longer than
    // HARD's linear cliff at the same nominal Release setting (~16 scope columns vs ~3, see
    // GATECRASHER-GUI-SPEC.md section 5) - so Soft scales the effective release duration as well
    // as curving it, rather than just curving the same duration.
    constexpr float softReleaseDurationMultiplier = 5.0f;
    constexpr float softReleaseCurveExponent = 2.2f;

    constexpr float minAudibleGain = 1.0e-4f;
}

void GateEnvelopeGenerator::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void GateEnvelopeGenerator::reset()
{
    state = State::closed;
    gain = 0.0f;
    holdCounterSamples = 0.0;
    releaseCounterSamples = 0.0;
    releaseStartGain = 0.0f;
    gateOpen.store(false, std::memory_order_relaxed);
    currentGain.store(0.0f, std::memory_order_relaxed);
}

void GateEnvelopeGenerator::process(const float* triggerLevel, int numSamples,
                                     float thresholdDb, float attackMs, float holdMs, float releaseMs,
                                     GateShape shape, float* outGain)
{
    const float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb, -100.0f);
    const double attackSamples = juce::jmax(1.0, 0.001 * attackMs * sampleRate);
    const double holdSamples = juce::jmax(0.0, 0.001 * holdMs * sampleRate);
    const double baseReleaseSamples = juce::jmax(1.0, 0.001 * releaseMs * sampleRate);
    const double releaseSamples = shape == GateShape::soft
                                       ? baseReleaseSamples * softReleaseDurationMultiplier
                                       : baseReleaseSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        const bool triggered = triggerLevel[i] >= thresholdLinear;

        switch (state)
        {
            case State::closed:
                if (triggered)
                {
                    state = State::attack;
                    gateOpen.store(true, std::memory_order_relaxed);
                }
                break;

            case State::attack:
                // Once triggered, attack always runs to completion - it can't be interrupted by
                // the trigger dropping back below threshold mid-ramp.
                gain += (float) (1.0 / attackSamples);
                if (gain >= 1.0f)
                {
                    gain = 1.0f;
                    state = State::hold;
                    holdCounterSamples = 0.0;
                }
                break;

            case State::hold:
                gain = 1.0f;
                if (triggered)
                    holdCounterSamples = 0.0; // retrigger keeps the gate open
                else
                    holdCounterSamples += 1.0;

                if (holdCounterSamples >= holdSamples)
                {
                    state = State::release;
                    releaseCounterSamples = 0.0;
                    releaseStartGain = gain;
                }
                break;

            case State::release:
                if (triggered)
                {
                    // Retrigger mid-release: resume opening from wherever the envelope currently
                    // sits rather than snapping back to 0 first, so there's no click.
                    state = State::attack;
                    break;
                }

                releaseCounterSamples += 1.0;
                {
                    const float t = juce::jlimit(0.0f, 1.0f,
                                                  (float) (releaseCounterSamples / releaseSamples));
                    const float shaped = shape == GateShape::soft
                                              ? std::pow(1.0f - t, softReleaseCurveExponent)
                                              : (1.0f - t);
                    gain = releaseStartGain * shaped;
                }

                if (gain <= minAudibleGain)
                {
                    gain = 0.0f;
                    state = State::closed;
                    gateOpen.store(false, std::memory_order_relaxed);
                }
                break;
        }

        outGain[i] = gain;
    }

    currentGain.store(gain, std::memory_order_relaxed);
}
