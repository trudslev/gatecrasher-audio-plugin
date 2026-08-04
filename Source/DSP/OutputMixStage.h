#pragma once

#include <juce_dsp/juce_dsp.h>

// Final dry/wet blend (Mix%) and output trim (dB) stage. wetBuffer is processed in place; dryBuffer
// is the pre-chain tap PluginProcessor captures before the reverb/gate/damping/slam/width chain
// runs. Stateless, so prepare/reset are no-ops - kept for interface consistency.
class OutputMixStage
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& wetBuffer, const juce::AudioBuffer<float>& dryBuffer,
                 float mixPercent, float trimDb);
};
