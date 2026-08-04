#include "TriggerDetector.h"

namespace
{
    // Fast enough to track a drum transient without adding audible detection lag ahead of the
    // gate's own Attack parameter; slow enough on release that the level doesn't chatter between
    // samples of a decaying transient.
    constexpr float detectorAttackMs = 0.2f;
    constexpr float detectorReleaseMs = 8.0f;

    float msToCoeff(double sampleRate, float ms)
    {
        return std::exp(-1.0f / (float) (0.001 * ms * sampleRate));
    }
}

void TriggerDetector::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;
    hpFilter.prepare(monoSpec);
    lpFilter.prepare(monoSpec);

    envAttackCoeff = msToCoeff(sampleRate, detectorAttackMs);
    envReleaseCoeff = msToCoeff(sampleRate, detectorReleaseMs);

    lastHPHz = -1.0f;
    lastLPHz = -1.0f;
    reset();
}

void TriggerDetector::reset()
{
    hpFilter.reset();
    lpFilter.reset();
    envelope = 0.0f;
}

void TriggerDetector::updateFilters(float hpHz, float lpHz)
{
    if (! juce::approximatelyEqual(hpHz, lastHPHz))
    {
        *hpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpHz);
        lastHPHz = hpHz;
    }

    if (! juce::approximatelyEqual(lpHz, lastLPHz))
    {
        *lpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpHz);
        lastLPHz = lpHz;
    }
}

void TriggerDetector::process(const juce::AudioBuffer<float>& mainInput,
                               const juce::AudioBuffer<float>* sidechainBuffer,
                               bool useSidechain,
                               float trigHPHz, float trigLPHz,
                               float* outLevel, int numSamples)
{
    updateFilters(trigHPHz, trigLPHz);

    const bool sidechainAvailable = useSidechain && sidechainBuffer != nullptr
                                     && sidechainBuffer->getNumSamples() >= numSamples
                                     && sidechainBuffer->getNumChannels() > 0;
    const auto& source = sidechainAvailable ? *sidechainBuffer : mainInput;
    const int numChannels = source.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += source.getSample(ch, i);
        mono = numChannels > 0 ? mono / (float) numChannels : 0.0f;

        mono = hpFilter.processSample(mono);
        mono = lpFilter.processSample(mono);

        const float rectified = std::abs(mono);
        const float coeff = rectified > envelope ? envAttackCoeff : envReleaseCoeff;
        envelope = rectified + coeff * (envelope - rectified);

        outLevel[i] = envelope;
    }
}
