#pragma once

#include <juce_dsp/juce_dsp.h>

// Mid/side stereo width control on the wet signal, 0-200% (100% = unprocessed). Stateless, so
// prepare/reset are no-ops - kept for interface consistency with the other DSP stages.
class StereoWidthStage
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float widthPercent);
};
