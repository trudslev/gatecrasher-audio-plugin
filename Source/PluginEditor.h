#pragma once

#include "PluginProcessor.h"
#include "GUI/GatecrasherEditorContent.h"
#include <juce_audio_processors/juce_audio_processors.h>

class GatecrasherAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GatecrasherAudioProcessorEditor(GatecrasherAudioProcessor&);
    ~GatecrasherAudioProcessorEditor() override;

    void resized() override;

private:
    GatecrasherAudioProcessor& processorRef;
    GatecrasherEditorContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GatecrasherAudioProcessorEditor)
};
