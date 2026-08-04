#include "TestUtils.h"
#include "../Source/DSP/SlamSaturation.h"
#include <cmath>

class SlamSaturationTests final : public juce::UnitTest
{
public:
    SlamSaturationTests() : juce::UnitTest("SlamSaturation", "DSP") {}

    void runTest() override
    {
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        const int numChannels = 2;
        juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32) blockSize, (juce::uint32) numChannels };

        beginTest("Output stays finite and bounded across the full Slam range");
        {
            for (float slamDb : { 0.0f, 3.0f, 7.0f, 12.0f })
            {
                SlamSaturation sat;
                sat.prepare(spec);
                auto buffer = generatePinkNoise(numChannels, blockSize, 321);

                for (int block = 0; block < 10; ++block)
                    sat.process(buffer, slamDb);

                for (int ch = 0; ch < numChannels; ++ch)
                    for (int i = 0; i < blockSize; ++i)
                    {
                        const float v = buffer.getReadPointer(ch)[i];
                        expect(std::isfinite(v), "Output must stay finite");
                        expect(std::abs(v) < 5.0f, "Output should stay in a sane bounded range at slam=" + juce::String(slamDb));
                    }
            }
        }

        beginTest("Slam = 0 stays close to unity gain on a moderate-level sine (mild inherent character only)");
        {
            SlamSaturation sat;
            sat.prepare(spec);

            juce::AudioBuffer<float> buffer(numChannels, blockSize);
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i)
                    data[i] = 0.3f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * (float) i / (float) sampleRate);
            }

            sat.process(buffer, 0.0f);

            float maxAbs = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                for (int i = 0; i < blockSize; ++i)
                    maxAbs = juce::jmax(maxAbs, std::abs(buffer.getReadPointer(ch)[i]));

            expectWithinAbsoluteError(maxAbs, 0.3f, 0.05f);
        }
    }
};

static SlamSaturationTests slamSaturationTests;
