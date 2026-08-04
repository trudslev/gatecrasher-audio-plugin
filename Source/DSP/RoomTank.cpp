#include "RoomTank.h"

void RoomTank::prepare(const juce::dsp::ProcessSpec& spec)
{
    CombAllpassNetwork::Config config;
    config.combDelaysMs = { 29.7f, 37.1f, 41.3f, 47.9f };
    config.allpassDelaysMs = { 5.3f, 1.7f };
    config.allpassFeedback = 0.5f;
    config.minCombFeedback = 0.55f;
    config.maxCombFeedback = 0.988f;
    config.maxSizeScale = 1.5f;
    network.prepare(spec, config);
}

void RoomTank::reset()
{
    network.reset();
}

void RoomTank::process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01)
{
    network.process(buffer, size01, decay01, density01);
}
