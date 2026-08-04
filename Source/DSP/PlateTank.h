#pragma once

#include "CombAllpassNetwork.h"
#include "ReverbTank.h"

// Dense, bright, tightly-spaced character - the classic plate signature that most gated-reverb
// records of this era actually used.
class PlateTank : public ReverbTank
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01) override;

private:
    CombAllpassNetwork network;
};
