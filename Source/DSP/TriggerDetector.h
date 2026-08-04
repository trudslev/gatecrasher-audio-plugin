#pragma once

#include <juce_dsp/juce_dsp.h>

// Reads either the main input or an optional sidechain bus, applies the Trigger HP/LP filters to
// the detection path only (never touches the audio signal itself), and produces a fast/smooth
// rectified envelope that GateEnvelopeGenerator thresholds against.
class TriggerDetector
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // sidechainBuffer may be nullptr when no sidechain bus is connected - useSidechain then
    // silently falls back to mainInput. Writes numSamples values into outLevel (linear, 0..~1).
    void process(const juce::AudioBuffer<float>& mainInput,
                 const juce::AudioBuffer<float>* sidechainBuffer,
                 bool useSidechain,
                 float trigHPHz, float trigLPHz,
                 float* outLevel, int numSamples);

private:
    void updateFilters(float hpHz, float lpHz);

    double sampleRate = 44100.0;

    juce::dsp::IIR::Filter<float> hpFilter;
    juce::dsp::IIR::Filter<float> lpFilter;
    float lastHPHz = -1.0f;
    float lastLPHz = -1.0f;

    float envelope = 0.0f;
    float envAttackCoeff = 0.0f;
    float envReleaseCoeff = 0.0f;
};
