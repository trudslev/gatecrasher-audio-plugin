#include "TestUtils.h"
#include "../Source/DSP/AmbienceTank.h"
#include "../Source/DSP/ChamberTank.h"
#include "../Source/DSP/PlateTank.h"
#include "../Source/DSP/ReverbEngine.h"
#include "../Source/DSP/RoomTank.h"
#include <array>
#include <cmath>
#include <memory>

namespace
{
    bool isFiniteAndBounded(const juce::AudioBuffer<float>& buffer, float maxAbs)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (! std::isfinite(data[i]))
                    return false;
                if (std::abs(data[i]) > maxAbs)
                    return false;
            }
        }
        return true;
    }
}

class ReverbTankTests final : public juce::UnitTest
{
public:
    ReverbTankTests() : juce::UnitTest("ReverbTank", "DSP") {}

    void runTest() override
    {
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        const int numChannels = 2;
        juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32) blockSize, (juce::uint32) numChannels };

        beginTest("Each algorithm stays silent when fed silence, even at max decay/density");
        {
            std::array<std::unique_ptr<ReverbTank>, 4> tanks{
                std::unique_ptr<ReverbTank>(std::make_unique<RoomTank>()),
                std::unique_ptr<ReverbTank>(std::make_unique<PlateTank>()),
                std::unique_ptr<ReverbTank>(std::make_unique<ChamberTank>()),
                std::unique_ptr<ReverbTank>(std::make_unique<AmbienceTank>())
            };

            for (auto& tank : tanks)
            {
                tank->prepare(spec);
                juce::AudioBuffer<float> buffer(numChannels, blockSize);
                buffer.clear();

                for (int block = 0; block < 20; ++block)
                    tank->process(buffer, 0.8f, 0.9f, 0.8f);

                expect(isFiniteAndBounded(buffer, 1.0e-3f), "Silence in should stay silence out, even at max decay/density");
            }
        }

        beginTest("Each algorithm produces a finite, bounded tail from an impulse at maximum decay/density/size");
        {
            std::array<std::unique_ptr<ReverbTank>, 4> tanks{
                std::unique_ptr<ReverbTank>(std::make_unique<RoomTank>()),
                std::unique_ptr<ReverbTank>(std::make_unique<PlateTank>()),
                std::unique_ptr<ReverbTank>(std::make_unique<ChamberTank>()),
                std::unique_ptr<ReverbTank>(std::make_unique<AmbienceTank>())
            };

            for (auto& tank : tanks)
            {
                tank->prepare(spec);
                juce::AudioBuffer<float> buffer(numChannels, blockSize);
                buffer.clear();
                buffer.setSample(0, 0, 1.0f);
                buffer.setSample(1, 0, 1.0f);

                for (int block = 0; block < 200; ++block)
                {
                    tank->process(buffer, 1.0f, 1.0f, 1.0f);
                    expect(isFiniteAndBounded(buffer, 50.0f),
                           "Tank output must stay finite and bounded, even at maximum decay/density/size");
                    if (block < 199)
                        buffer.clear();
                }
            }
        }

        beginTest("ReverbEngine switching algorithms produces no NaN/Inf and stays bounded through the crossfade");
        {
            ReverbEngine engine;
            engine.prepare(spec);

            juce::AudioBuffer<float> buffer(numChannels, blockSize);

            for (int block = 0; block < 10; ++block)
            {
                buffer = generatePinkNoise(numChannels, blockSize, 1000 + block);
                engine.process(buffer, ReverbAlgorithm::plate, 0.7f, 0.7f, 0.7f);
            }

            buffer = generatePinkNoise(numChannels, blockSize, 2000);
            engine.process(buffer, ReverbAlgorithm::room, 0.7f, 0.7f, 0.7f);
            expect(isFiniteAndBounded(buffer, 10.0f), "Output must stay finite and bounded through an algorithm switch");

            for (int block = 0; block < 10; ++block)
            {
                buffer = generatePinkNoise(numChannels, blockSize, 3000 + block);
                engine.process(buffer, ReverbAlgorithm::room, 0.7f, 0.7f, 0.7f);
                expect(isFiniteAndBounded(buffer, 10.0f), "Output must stay finite and bounded after the switch settles");
            }
        }
    }
};

static ReverbTankTests reverbTankTests;
