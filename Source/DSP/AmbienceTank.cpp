#include "AmbienceTank.h"

void AmbienceTank::prepare(const juce::dsp::ProcessSpec& spec)
{
    CombAllpassNetwork::Config config;
    config.combDelaysMs = { 7.9f, 11.3f, 13.7f };
    config.allpassDelaysMs = { 1.1f };
    config.allpassFeedback = 0.45f;
    config.minCombFeedback = 0.40f;
    config.maxCombFeedback = 0.93f; // deliberately capped low - Ambience never grows a long tail
    config.maxSizeScale = 1.2f;
    network.prepare(spec, config);
}

void AmbienceTank::reset()
{
    network.reset();
}

void AmbienceTank::process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01)
{
    network.process(buffer, size01, decay01, density01);
}
