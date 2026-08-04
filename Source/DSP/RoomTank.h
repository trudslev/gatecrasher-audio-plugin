#pragma once

#include "CombAllpassNetwork.h"
#include "ReverbTank.h"

// Discrete, spaced-out early-reflection character - the most "real space" sounding of the four
// algorithms, and the longest available decay.
class RoomTank : public ReverbTank
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01) override;

private:
    CombAllpassNetwork network;
};
