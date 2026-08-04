#pragma once

#include <juce_dsp/juce_dsp.h>

// Drive/saturation stage applied to the wet (gated) signal - the "Slam" character control, 0 to
// +12 driving a tanh waveshaper (not a literal output-level trim; that's OutputMixStage's job).
class SlamSaturation
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float slamDb);

private:
    juce::dsp::WaveShaper<float> shaper { [] (float x) { return std::tanh(x); } };
};
