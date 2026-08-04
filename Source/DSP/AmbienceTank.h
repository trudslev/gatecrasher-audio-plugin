#pragma once

#include "CombAllpassNetwork.h"
#include "ReverbTank.h"

// Short, early-reflections-only character - even at maximum Decay this never grows a long tail,
// making it the choice for adding size without washing out phrasing.
class AmbienceTank : public ReverbTank
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01) override;

private:
    CombAllpassNetwork network;
};
