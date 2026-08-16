#pragma once

#include "AmbienceTank.h"
#include "ChamberTank.h"
#include "PlateTank.h"
#include "RoomTank.h"

enum class ReverbAlgorithm { room, plate, chamber, ambience };

// Owns one instance of each ReverbTank algorithm and forwards Size/Decay/Density to whichever is
// active. Crossfades briefly across an algorithm switch so changing Algorithm mid-performance
// doesn't click (mirrors TapeRot's MODEL-switch click-avoidance).
class ReverbEngine
{
public:
    /*  **`initialAlgorithm` is an argument, and that is the fix rather than a convenience.**

        `currentAlgorithm` used to be constructed to `plate` and never reconciled, while the default
        Program selects ROOM — so the first block of every instance found the requested tank
        different from the stored one and started a crossfade nobody asked for. Measured at 0.041.

        **A stored copy of a selection compared per block is correct or incorrect depending on what
        its branch DOES**, and this suite carries both kinds in this casting. Guarding a coefficient
        RECOMPUTE, it should start at an impossible value so it fires immediately —
        `DampingStage::lastHF01 = -1.0f` is exactly that and is right as it stands; do not "tidy" it
        to match this. Guarding a TRANSITION, as here, it must start at what the configuration
        selects, because a crossfade out of a state nobody chose is audible.

        Same construction, opposite correct initial value. There is no name that distinguishes them,
        which is why each has to be read for what its branch does. */
    void prepare(const juce::dsp::ProcessSpec& spec, ReverbAlgorithm initialAlgorithm);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, ReverbAlgorithm algorithm,
                 float size01, float decay01, float density01);

private:
    ReverbTank* tankFor(ReverbAlgorithm algorithm) noexcept;

    RoomTank room;
    PlateTank plate;
    ChamberTank chamber;
    AmbienceTank ambience;

    ReverbAlgorithm currentAlgorithm = ReverbAlgorithm::plate;
    juce::AudioBuffer<float> previousTankOutput;
    juce::SmoothedValue<float> switchCrossfade { 1.0f };
    static constexpr double switchCrossfadeSeconds = 0.06;
    double sampleRate = 44100.0;
};
