#include "TestUtils.h"
#include "../Source/DSP/TriggerDetector.h"
#include <cmath>
#include <vector>

namespace
{
    juce::AudioBuffer<float> generateSine(int numChannels, int numSamples, double sampleRate, float freqHz, float amplitude)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = amplitude * (float) std::sin(2.0 * juce::MathConstants<double>::pi * freqHz * (double) i / sampleRate);
        }
        return buffer;
    }
}

class TriggerDetectorTests final : public juce::UnitTest
{
public:
    TriggerDetectorTests() : juce::UnitTest("TriggerDetector", "DSP") {}

    void runTest() override
    {
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        const int numChannels = 2;
        juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32) blockSize, (juce::uint32) numChannels };

        beginTest("Silence produces a near-zero, finite envelope");
        {
            TriggerDetector detector;
            detector.prepare(spec);

            juce::AudioBuffer<float> silence(numChannels, blockSize);
            silence.clear();
            std::vector<float> level((size_t) blockSize);

            for (int block = 0; block < 10; ++block)
                detector.process(silence, nullptr, false, 180.0f, 6300.0f, level.data(), blockSize);

            for (float v : level)
            {
                expect(std::isfinite(v), "Envelope must stay finite on silence");
                expect(v < 0.001f, "Envelope should decay to near zero on silence");
            }
        }

        beginTest("A loud in-band sine produces a bounded, non-zero envelope");
        {
            TriggerDetector detector;
            detector.prepare(spec);

            auto sine = generateSine(numChannels, blockSize, sampleRate, 1000.0f, 0.9f);
            std::vector<float> level((size_t) blockSize);

            float lastLevel = 0.0f;
            for (int block = 0; block < 20; ++block)
            {
                detector.process(sine, nullptr, false, 180.0f, 6300.0f, level.data(), blockSize);
                lastLevel = level.back();
            }

            expect(std::isfinite(lastLevel), "Envelope must stay finite");
            expect(lastLevel > 0.1f, "A loud in-band sine should produce a substantial envelope level");
            expect(lastLevel < 2.0f, "Envelope should stay in a sane bounded range");
        }

        beginTest("A tone well below Trigger HP is substantially attenuated relative to an in-band tone");
        {
            TriggerDetector below;
            TriggerDetector inBand;
            below.prepare(spec);
            inBand.prepare(spec);

            auto lowSine = generateSine(numChannels, blockSize, sampleRate, 20.0f, 0.9f);
            auto midSine = generateSine(numChannels, blockSize, sampleRate, 1000.0f, 0.9f);
            std::vector<float> lowLevel((size_t) blockSize), midLevel((size_t) blockSize);

            float lastLow = 0.0f, lastMid = 0.0f;
            for (int block = 0; block < 20; ++block)
            {
                below.process(lowSine, nullptr, false, 500.0f, 6300.0f, lowLevel.data(), blockSize);
                inBand.process(midSine, nullptr, false, 500.0f, 6300.0f, midLevel.data(), blockSize);
                lastLow = lowLevel.back();
                lastMid = midLevel.back();
            }

            expect(lastLow < lastMid * 0.5f,
                   "A 20Hz tone should be well attenuated by a 500Hz Trigger HP filter relative to an in-band tone");
        }

        beginTest("useSidechain with a null sidechain buffer falls back to the main input");
        {
            TriggerDetector withNullSidechain;
            TriggerDetector mainOnly;
            withNullSidechain.prepare(spec);
            mainOnly.prepare(spec);

            auto sine = generateSine(numChannels, blockSize, sampleRate, 1000.0f, 0.5f);
            std::vector<float> levelA((size_t) blockSize), levelB((size_t) blockSize);

            withNullSidechain.process(sine, nullptr, true, 180.0f, 6300.0f, levelA.data(), blockSize);
            mainOnly.process(sine, nullptr, false, 180.0f, 6300.0f, levelB.data(), blockSize);

            for (int i = 0; i < blockSize; ++i)
                expectWithinAbsoluteError(levelA[(size_t) i], levelB[(size_t) i], 1.0e-6f);
        }
    }
};

static TriggerDetectorTests triggerDetectorTests;
