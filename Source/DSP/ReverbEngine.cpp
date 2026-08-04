#include "ReverbEngine.h"

void ReverbEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    room.prepare(spec);
    plate.prepare(spec);
    chamber.prepare(spec);
    ambience.prepare(spec);

    previousTankOutput.setSize((int) spec.numChannels, (int) spec.maximumBlockSize);
    switchCrossfade.reset(sampleRate, switchCrossfadeSeconds);
    switchCrossfade.setCurrentAndTargetValue(1.0f);
    reset();
}

void ReverbEngine::reset()
{
    room.reset();
    plate.reset();
    chamber.reset();
    ambience.reset();
    previousTankOutput.clear();
}

ReverbTank* ReverbEngine::tankFor(ReverbAlgorithm algorithm) noexcept
{
    switch (algorithm)
    {
        case ReverbAlgorithm::room:     return &room;
        case ReverbAlgorithm::plate:    return &plate;
        case ReverbAlgorithm::chamber:  return &chamber;
        case ReverbAlgorithm::ambience: return &ambience;
    }
    return &plate;
}

void ReverbEngine::process(juce::AudioBuffer<float>& buffer, ReverbAlgorithm algorithm,
                            float size01, float decay01, float density01)
{
    if (algorithm != currentAlgorithm)
    {
        // Snapshot what the *previous* algorithm would have produced for this block's dry input,
        // then swap - the crossfade below blends from this snapshot into the newly-selected tank's
        // output over switchCrossfadeSeconds.
        previousTankOutput.makeCopyOf(buffer, true);
        tankFor(currentAlgorithm)->process(previousTankOutput, size01, decay01, density01);

        currentAlgorithm = algorithm;
        switchCrossfade.setCurrentAndTargetValue(0.0f);
        switchCrossfade.setTargetValue(1.0f);
    }

    tankFor(currentAlgorithm)->process(buffer, size01, decay01, density01);

    if (! switchCrossfade.isSmoothing() && switchCrossfade.getCurrentValue() >= 1.0f)
        return;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float mix = switchCrossfade.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            const float prev = ch < previousTankOutput.getNumChannels()
                                    ? previousTankOutput.getSample(ch, i)
                                    : 0.0f;
            data[i] = prev + (data[i] - prev) * mix;
        }
    }
}
