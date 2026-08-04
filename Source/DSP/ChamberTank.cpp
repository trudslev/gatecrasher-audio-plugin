#include "ChamberTank.h"

void ChamberTank::prepare(const juce::dsp::ProcessSpec& spec)
{
    CombAllpassNetwork::Config config;
    config.combDelaysMs = { 23.1f, 29.7f, 34.3f, 39.1f, 43.7f };
    config.allpassDelaysMs = { 4.1f, 2.3f };
    config.allpassFeedback = 0.55f;
    config.minCombFeedback = 0.55f;
    config.maxCombFeedback = 0.985f;
    config.maxSizeScale = 1.25f;
    network.prepare(spec, config);
}

void ChamberTank::reset()
{
    network.reset();
}

void ChamberTank::process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01)
{
    network.process(buffer, size01, decay01, density01);
}
