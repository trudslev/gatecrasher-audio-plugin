#pragma once

#include <vector>
#include <juce_dsp/juce_dsp.h>

// Post-tank HF/LF tone shaping applied to the wet reverb signal, before the gate multiply - shapes
// the character of the tail that then gets chopped. Implemented as simple post-filters rather than
// per-band decay inside each ReverbTank's feedback loop - simpler and independently testable, at
// the cost of not being physically how real hardware damping works (which shapes decay rate per
// band, not just the current signal's tone). Flagged as an accepted architectural simplification.
class DampingStage
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float dampHF01, float dampLF01);

private:
    static constexpr float minHFCutoffHz = 1500.0f;
    static constexpr float maxHFCutoffHz = 18000.0f;
    static constexpr float minLFCutoffHz = 40.0f;
    static constexpr float maxLFCutoffHz = 500.0f;

    void updateCoefficients(float dampHF01, float dampLF01);

    double sampleRate = 44100.0;
    std::vector<juce::dsp::IIR::Filter<float>> hfFilters;
    std::vector<juce::dsp::IIR::Filter<float>> lfFilters;
    float lastHF01 = -1.0f;
    float lastLF01 = -1.0f;
};
