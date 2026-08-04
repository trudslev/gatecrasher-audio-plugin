#include "StereoWidthStage.h"

void StereoWidthStage::prepare(const juce::dsp::ProcessSpec&) {}
void StereoWidthStage::reset() {}

void StereoWidthStage::process(juce::AudioBuffer<float>& buffer, float widthPercent)
{
    if (buffer.getNumChannels() < 2)
        return;

    const float width = juce::jlimit(0.0f, 2.0f, widthPercent * 0.01f);
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float mid = 0.5f * (left[i] + right[i]);
        const float side = 0.5f * (left[i] - right[i]) * width;
        left[i] = mid + side;
        right[i] = mid - side;
    }
}
