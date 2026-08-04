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
    void prepare(const juce::dsp::ProcessSpec& spec);
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
