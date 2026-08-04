#pragma once

#include <algorithm>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// Shared comb+allpass diffusion network used internally by each ReverbTank algorithm
// (RoomTank/PlateTank/ChamberTank/AmbienceTank differ only in the delay-length tables, comb/
// allpass counts, and feedback range passed via Config) - an implementation-sharing detail, not a
// DSP class of its own, the same way TapeRot's DegradationCore composes WowFlutter/TapeModelEQ/
// NoiseSource without those becoming public per-algorithm surface area.
//
// Classic Schroeder/Moorer topology: N parallel feedback combs summed, then M series allpasses for
// diffusion. This is a first, functioning pass, not an acoustically tuned one - the values here are
// starting points, refined by ear later, the same way TapeRot's tape-model EQ curves were (see
// BUILDING.md's "what the test suite covers" note on tonal correctness being a manual pass).
class CombAllpassNetwork
{
public:
    struct Config
    {
        std::vector<float> combDelaysMs;
        std::vector<float> allpassDelaysMs;
        float minCombFeedback = 0.55f;   // comb feedback at decay01 = 0
        float maxCombFeedback = 0.985f;  // comb feedback at decay01 = 1 (never allowed to reach 1)
        float allpassFeedback = 0.5f;    // allpass feedback at density01 = 1
        float maxSizeScale = 1.35f;      // how far Size can stretch delay lengths above nominal
    };

    void prepare(const juce::dsp::ProcessSpec& spec, Config configIn)
    {
        config = std::move(configIn);
        sampleRate = spec.sampleRate;
        numChannels = (int) spec.numChannels;

        const float maxCombMs = *std::max_element(config.combDelaysMs.begin(), config.combDelaysMs.end());
        const float maxAllpassMs = *std::max_element(config.allpassDelaysMs.begin(), config.allpassDelaysMs.end());
        const int maxCombSamples = (int) (sampleRate * 0.001 * maxCombMs * config.maxSizeScale) + 4;
        const int maxAllpassSamples = (int) (sampleRate * 0.001 * maxAllpassMs * config.maxSizeScale) + 4;

        juce::dsp::ProcessSpec lineSpec{ spec.sampleRate, spec.maximumBlockSize, 1 };

        combLines.clear();
        allpassLines.clear();
        combLines.resize((size_t) numChannels);
        allpassLines.resize((size_t) numChannels);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (size_t c = 0; c < config.combDelaysMs.size(); ++c)
            {
                juce::dsp::DelayLine<float> line(maxCombSamples);
                line.prepare(lineSpec);
                combLines[(size_t) ch].push_back(std::move(line));
            }
            for (size_t a = 0; a < config.allpassDelaysMs.size(); ++a)
            {
                juce::dsp::DelayLine<float> line(maxAllpassSamples);
                line.prepare(lineSpec);
                allpassLines[(size_t) ch].push_back(std::move(line));
            }
        }

        reset();
    }

    void reset()
    {
        for (auto& perChannel : combLines)
            for (auto& line : perChannel)
                line.reset();
        for (auto& perChannel : allpassLines)
            for (auto& line : perChannel)
                line.reset();
    }

    void process(juce::AudioBuffer<float>& buffer, float size01, float decay01, float density01)
    {
        const float sizeScale = juce::jmap(juce::jlimit(0.0f, 1.0f, size01), 0.4f, config.maxSizeScale);
        const float combFeedback = juce::jmap(juce::jlimit(0.0f, 1.0f, decay01),
                                               config.minCombFeedback, config.maxCombFeedback);
        const float allpassFeedback = juce::jmap(juce::jlimit(0.0f, 1.0f, density01),
                                                  config.allpassFeedback * 0.5f, config.allpassFeedback);

        const int numSamples = buffer.getNumSamples();
        const int numCombs = (int) config.combDelaysMs.size();
        const int numAllpasses = (int) config.allpassDelaysMs.size();

        for (int ch = 0; ch < juce::jmin(numChannels, buffer.getNumChannels()); ++ch)
        {
            auto& combsForChannel = combLines[(size_t) ch];
            auto& allpassesForChannel = allpassLines[(size_t) ch];

            for (int c = 0; c < numCombs; ++c)
                combsForChannel[(size_t) c].setDelay(
                    (float) (sampleRate * 0.001 * config.combDelaysMs[(size_t) c] * sizeScale));
            for (int a = 0; a < numAllpasses; ++a)
                allpassesForChannel[(size_t) a].setDelay(
                    (float) (sampleRate * 0.001 * config.allpassDelaysMs[(size_t) a] * sizeScale));

            auto* data = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const float input = data[i];

                float combSum = 0.0f;
                for (int c = 0; c < numCombs; ++c)
                {
                    auto& line = combsForChannel[(size_t) c];
                    const float delayed = line.popSample(0);
                    line.pushSample(0, input + delayed * combFeedback);
                    combSum += delayed;
                }
                combSum /= (float) juce::jmax(1, numCombs);

                float diffused = combSum;
                for (int a = 0; a < numAllpasses; ++a)
                {
                    auto& line = allpassesForChannel[(size_t) a];
                    const float delayed = line.popSample(0);
                    const float apOut = -diffused * allpassFeedback + delayed;
                    line.pushSample(0, diffused + apOut * allpassFeedback);
                    diffused = apOut;
                }

                data[i] = diffused;
            }
        }
    }

private:
    Config config;
    double sampleRate = 44100.0;
    int numChannels = 2;

    std::vector<std::vector<juce::dsp::DelayLine<float>>> combLines;
    std::vector<std::vector<juce::dsp::DelayLine<float>>> allpassLines;
};
