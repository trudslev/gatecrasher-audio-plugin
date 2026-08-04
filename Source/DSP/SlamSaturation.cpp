#include "SlamSaturation.h"

void SlamSaturation::prepare(const juce::dsp::ProcessSpec& spec)
{
    shaper.prepare(spec);
    reset();
}

void SlamSaturation::reset()
{
    shaper.reset();
}

void SlamSaturation::process(juce::AudioBuffer<float>& buffer, float slamDb)
{
    // output = tanh(driveGain*x) / tanh(driveGain) - a standard drive/makeup pair whose full-scale
    // peak (x=1) always maps to unity gain, at any driveGain. The reason driveGain needs to range
    // down to near-zero (not floor at 1.0, as an earlier version of this did) is that
    // tanh(g*x)/tanh(g) -> x as g -> 0 for *any* x, not just x=1 (Taylor expand tanh at 0) - that's
    // what makes Slam=0 genuinely transparent for real (non-full-scale) program material instead of
    // boosting quieter passages ~25-30%, which is what a driveGain floor of 1.0 was doing (see
    // SlamSaturationTests.cpp's "close to unity gain" case, which caught this).
    const float driveGain = juce::jmap(juce::jlimit(0.0f, 12.0f, slamDb), 0.0f, 12.0f, 0.02f, 4.0f);
    const float makeupGain = 1.0f / std::tanh(driveGain);

    juce::dsp::AudioBlock<float> block(buffer);
    block.multiplyBy(driveGain);
    juce::dsp::ProcessContextReplacing<float> context(block);
    shaper.process(context);
    block.multiplyBy(makeupGain);
}
