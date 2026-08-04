#include "TestUtils.h"
#include "../Source/DSP/StereoWidthStage.h"

class StereoWidthStageTests final : public juce::UnitTest
{
public:
    StereoWidthStageTests() : juce::UnitTest("StereoWidthStage", "DSP") {}

    void runTest() override
    {
        const int blockSize = 64;
        juce::dsp::ProcessSpec spec{ 48000.0, (juce::uint32) blockSize, 2 };

        beginTest("width = 100% leaves the signal unchanged");
        {
            StereoWidthStage stage;
            stage.prepare(spec);

            juce::AudioBuffer<float> buffer(2, blockSize);
            for (int i = 0; i < blockSize; ++i)
            {
                buffer.setSample(0, i, 0.5f + 0.01f * (float) i);
                buffer.setSample(1, i, -0.3f + 0.02f * (float) i);
            }
            auto original = buffer;

            stage.process(buffer, 100.0f);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                    expectWithinAbsoluteError(buffer.getSample(ch, i), original.getSample(ch, i), 1.0e-5f);
        }

        beginTest("width = 0% collapses left and right to identical (mono) signals");
        {
            StereoWidthStage stage;
            stage.prepare(spec);

            juce::AudioBuffer<float> buffer(2, blockSize);
            for (int i = 0; i < blockSize; ++i)
            {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, -0.3f);
            }

            stage.process(buffer, 0.0f);

            for (int i = 0; i < blockSize; ++i)
                expectWithinAbsoluteError(buffer.getSample(0, i), buffer.getSample(1, i), 1.0e-5f);
        }

        beginTest("width = 200% doubles the side component relative to width = 100%");
        {
            StereoWidthStage stage100, stage200;
            stage100.prepare(spec);
            stage200.prepare(spec);

            juce::AudioBuffer<float> buffer100(2, blockSize), buffer200(2, blockSize);
            for (int i = 0; i < blockSize; ++i)
            {
                buffer100.setSample(0, i, 0.5f);
                buffer100.setSample(1, i, -0.3f);
                buffer200.setSample(0, i, 0.5f);
                buffer200.setSample(1, i, -0.3f);
            }

            stage100.process(buffer100, 100.0f);
            stage200.process(buffer200, 200.0f);

            const float side100 = 0.5f * (buffer100.getSample(0, 0) - buffer100.getSample(1, 0));
            const float side200 = 0.5f * (buffer200.getSample(0, 0) - buffer200.getSample(1, 0));
            expectWithinAbsoluteError(side200, side100 * 2.0f, 1.0e-4f);
        }

        beginTest("A mono buffer is left untouched, not crashed on");
        {
            StereoWidthStage stage;
            stage.prepare(spec);

            juce::AudioBuffer<float> mono(1, blockSize);
            mono.setSample(0, 0, 0.42f);
            stage.process(mono, 150.0f);

            expectWithinAbsoluteError(mono.getSample(0, 0), 0.42f, 1.0e-6f);
        }
    }
};

static StereoWidthStageTests stereoWidthStageTests;
