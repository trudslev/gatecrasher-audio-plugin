#pragma once

#include <juce_dsp/juce_dsp.h>

// Interface implemented by each reverb algorithm (RoomTank/PlateTank/ChamberTank/AmbienceTank).
// Each owns its own delay-network topology; ReverbEngine owns one instance of each and forwards
// Size/Decay/Density to whichever is currently active.
class ReverbTank
{
public:
    virtual ~ReverbTank() = default;

    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;
    virtual void reset() = 0;

    // Processes buffer in place. size01/decay01/density01 are all normalised 0-1.
    virtual void process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01) = 0;
};
