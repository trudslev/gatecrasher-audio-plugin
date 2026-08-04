#pragma once

#include "CombAllpassNetwork.h"
#include "ReverbTank.h"

// Balanced between RoomTank's discrete spaciousness and PlateTank's dense brightness - a mid-sized,
// moderately diffuse character.
class ChamberTank : public ReverbTank
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01) override;

private:
    CombAllpassNetwork network;
};
