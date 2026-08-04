#include "DampingStage.h"

void DampingStage::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    // resize(), not assign(count, {}): juce::dsp::IIR::Filter isn't copy-assignable (it holds a
    // reference-counted Coefficients::Ptr plus internal state), so filling multiple vector slots
    // from one copy-constructed temporary doesn't compile. resize() default-constructs each slot
    // in place instead.
    hfFilters.resize((size_t) spec.numChannels);
    lfFilters.resize((size_t) spec.numChannels);
    for (auto& f : hfFilters) f.prepare(monoSpec);
    for (auto& f : lfFilters) f.prepare(monoSpec);

    lastHF01 = -1.0f;
    lastLF01 = -1.0f;
    reset();
}

void DampingStage::reset()
{
    for (auto& f : hfFilters) f.reset();
    for (auto& f : lfFilters) f.reset();
}

void DampingStage::updateCoefficients(float dampHF01, float dampLF01)
{
    // dampHF01 = 0 -> filter fully open (max cutoff, transparent); 1 -> darkest (min cutoff).
    // dampLF01 mirrors this on the low end (0 = transparent, 1 = thinnest/tightest).
    if (! juce::approximatelyEqual(dampHF01, lastHF01))
    {
        const float cutoff = juce::jmap(juce::jlimit(0.0f, 1.0f, dampHF01), maxHFCutoffHz, minHFCutoffHz);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoff);
        for (auto& f : hfFilters) *f.coefficients = *coeffs;
        lastHF01 = dampHF01;
    }

    if (! juce::approximatelyEqual(dampLF01, lastLF01))
    {
        const float cutoff = juce::jmap(juce::jlimit(0.0f, 1.0f, dampLF01), minLFCutoffHz, maxLFCutoffHz);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoff);
        for (auto& f : lfFilters) *f.coefficients = *coeffs;
        lastLF01 = dampLF01;
    }
}

void DampingStage::process(juce::AudioBuffer<float>& buffer, float dampHF01, float dampLF01)
{
    updateCoefficients(dampHF01, dampLF01);

    const int numChannels = juce::jmin((int) hfFilters.size(), buffer.getNumChannels());
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = lfFilters[(size_t) ch].processSample(hfFilters[(size_t) ch].processSample(data[i]));
    }
}
