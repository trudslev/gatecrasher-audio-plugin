#include "TestUtils.h"
#include "../Source/DSP/DampingStage.h"
#include "../Source/DSP/GateEnvelopeGenerator.h"
#include "../Source/DSP/OutputMixStage.h"
#include "../Source/DSP/ReverbEngine.h"
#include "../Source/DSP/SlamSaturation.h"
#include "../Source/DSP/StereoWidthStage.h"
#include "../Source/DSP/TriggerDetector.h"
#include <vector>

class CPUCheckTests final : public juce::UnitTest
{
public:
    CPUCheckTests() : juce::UnitTest("CPUCheck", "Performance") {}

    void runTest() override
    {
        const double sampleRate = 48000.0;
        const int blockSize = 64;
        const int numChannels = 2;
        const int numIterations = 5000;
        juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32) blockSize, (juce::uint32) numChannels };
        const double realTimeBudgetMs = ((double) blockSize / sampleRate) * 1000.0;

        beginTest("Full signal chain stays within real-time budget at 48kHz/64 samples, max decay/density");
        {
            TriggerDetector trigger;
            GateEnvelopeGenerator gate;
            ReverbEngine reverb;
            DampingStage damping;
            SlamSaturation slam;
            StereoWidthStage width;
            OutputMixStage outputMix;

            trigger.prepare(spec);
            gate.prepare(sampleRate);
            reverb.prepare(spec);
            damping.prepare(spec);
            slam.prepare(spec);
            width.prepare(spec);
            outputMix.prepare(spec);

            auto buffer = generatePinkNoise(numChannels, blockSize, 606);
            auto dryBuffer = buffer;
            std::vector<float> triggerLevel((size_t) blockSize), gain((size_t) blockSize);

            const double start = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < numIterations; ++i)
            {
                trigger.process(buffer, nullptr, false, 180.0f, 6300.0f, triggerLevel.data(), blockSize);
                gate.process(triggerLevel.data(), blockSize, -18.5f, 0.4f, 165.0f, 4.0f, GateShape::soft, gain.data());
                reverb.process(buffer, ReverbAlgorithm::plate, 1.0f, 1.0f, 1.0f);
                damping.process(buffer, 0.5f, 0.5f);

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    auto* data = buffer.getWritePointer(ch);
                    for (int s = 0; s < blockSize; ++s)
                        data[s] *= gain[(size_t) s];
                }

                slam.process(buffer, 7.0f);
                width.process(buffer, 128.0f);
                outputMix.process(buffer, dryBuffer, 64.0f, -1.4f);
            }
            const double end = juce::Time::getMillisecondCounterHiRes();

            const double avgMsPerBlock = (end - start) / (double) numIterations;
            logMessage("Average block time: " + juce::String(avgMsPerBlock, 4) + " ms (budget: "
                       + juce::String(realTimeBudgetMs, 4) + " ms)");
            logMessage("CPU load: " + juce::String(100.0 * avgMsPerBlock / realTimeBudgetMs, 2) + "%");

            expect(avgMsPerBlock < realTimeBudgetMs,
                   "Full chain should stay below the real-time budget, even at max decay/density");
        }
    }
};

static CPUCheckTests cpuCheckTests;
