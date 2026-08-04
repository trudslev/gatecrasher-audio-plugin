#include "PlateTank.h"

void PlateTank::prepare(const juce::dsp::ProcessSpec& spec)
{
    CombAllpassNetwork::Config config;
    config.combDelaysMs = { 13.3f, 17.1f, 19.7f, 23.1f, 26.7f, 29.3f };
    config.allpassDelaysMs = { 3.1f, 1.3f, 0.7f };
    config.allpassFeedback = 0.62f;
    config.minCombFeedback = 0.60f;
    config.maxCombFeedback = 0.982f;
    config.maxSizeScale = 1.1f;
    network.prepare(spec, config);
}

void PlateTank::reset()
{
    network.reset();
}

void PlateTank::process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01)
{
    network.process(buffer, size01, decay01, density01);
}
