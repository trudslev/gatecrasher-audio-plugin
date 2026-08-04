#include "TestUtils.h"
#include "../Source/DSP/DampingStage.h"
#include <cmath>

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

    float rms(const juce::AudioBuffer<float>& buffer)
    {
        double sum = 0.0;
        int count = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                sum += (double) data[i] * (double) data[i];
                ++count;
            }
        }
        return (float) std::sqrt(sum / juce::jmax(1, count));
    }
}

class DampingStageTests final : public juce::UnitTest
{
public:
    DampingStageTests() : juce::UnitTest("DampingStage", "DSP") {}

    void runTest() override
    {
        const double sampleRate = 48000.0;
        const int blockSize = 2048;
        const int numChannels = 2;
        juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32) blockSize, (juce::uint32) numChannels };

        beginTest("dampHF = 1 attenuates a high-frequency tone relative to dampHF = 0");
        {
            DampingStage open, dark;
            open.prepare(spec);
            dark.prepare(spec);

            auto highTone = generateSine(numChannels, blockSize, sampleRate, 15000.0f, 0.8f);
            auto bufferOpen = highTone;
            auto bufferDark = highTone;

            for (int i = 0; i < 5; ++i)
            {
                open.process(bufferOpen, 0.0f, 0.0f);
                dark.process(bufferDark, 1.0f, 0.0f);
            }

            expect(rms(bufferDark) < rms(bufferOpen) * 0.5f,
                   "A 15kHz tone should be substantially attenuated at dampHF=1 relative to dampHF=0");
        }

        beginTest("dampLF = 1 attenuates a low-frequency tone relative to dampLF = 0");
        {
            DampingStage open, thin;
            open.prepare(spec);
            thin.prepare(spec);

            auto lowTone = generateSine(numChannels, blockSize, sampleRate, 60.0f, 0.8f);
            auto bufferOpen = lowTone;
            auto bufferThin = lowTone;

            for (int i = 0; i < 5; ++i)
            {
                open.process(bufferOpen, 0.0f, 0.0f);
                thin.process(bufferThin, 0.0f, 1.0f);
            }

            expect(rms(bufferThin) < rms(bufferOpen) * 0.5f,
                   "A 60Hz tone should be substantially attenuated at dampLF=1 relative to dampLF=0");
        }

        beginTest("Output stays finite on noise across the full damping range");
        {
            DampingStage stage;
            stage.prepare(spec);
            auto buffer = generatePinkNoise(numChannels, blockSize, 55);

            stage.process(buffer, 0.5f, 0.5f);

            for (int ch = 0; ch < numChannels; ++ch)
                for (int i = 0; i < blockSize; ++i)
                    expect(std::isfinite(buffer.getReadPointer(ch)[i]), "Output must stay finite");
        }
    }
};

static DampingStageTests dampingStageTests;
