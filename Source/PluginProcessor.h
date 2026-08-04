#pragma once

#include "Parameters.h"
#include "DSP/DampingStage.h"
#include "DSP/GateEnvelopeGenerator.h"
#include "DSP/OutputMixStage.h"
#include "DSP/ProgramManager.h"
#include "DSP/ReverbEngine.h"
#include "DSP/SlamSaturation.h"
#include "DSP/StereoWidthStage.h"
#include "DSP/TriggerDetector.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class GatecrasherAudioProcessor final : public juce::AudioProcessor
{
public:
    GatecrasherAudioProcessor();
    ~GatecrasherAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    // Rough estimate, not a hard bound: the reverb tanks are feedback networks whose actual decay
    // time varies continuously with the automatable Size/Decay parameters, up to a genuinely long
    // tail at their maxima (see Wall of Sound/Detonator) - this is a conservative value for hosts
    // that use it for freeze/bounce trailing-silence purposes, not a literal measured RT60.
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return programManager.getNumPrograms(); }
    int getCurrentProgram() override { return programManager.getCurrentProgram(); }
    void setCurrentProgram(int index) override { programManager.requestProgramChange(index); }
    const juce::String getProgramName(int index) override { return programManager.getProgramName(index); }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Factory programs (indices [0, kNumFactoryPrograms)) are the read-only, always-present
    // entries in kFactoryPrograms; user programs (indices [kNumFactoryPrograms, getNumPrograms()))
    // are files in ProgramManager's program directory. "Save" is never in-place for a factory
    // program, and never overwrites an existing user program either - it always creates a new one.
    bool isFactoryProgram(int index) const noexcept { return programManager.isFactoryProgram(index); }
    void saveNewUserProgram(const juce::String& name) { programManager.saveNewUserProgram(name); }
    void deleteUserProgram(int index) { programManager.deleteUserProgram(index); }

    juce::AudioProcessorValueTreeState apvts;

    // GUI-facing derived-display state, mirroring TapeRot's poll-based pattern. The scope/meter
    // components poll these once per repaint and accumulate their own scrolling history locally,
    // rather than the audio thread maintaining a shared history ring buffer - simpler, and
    // sufficient since the GUI redraws far more often than the display needs to be exact.
    bool isGateOpen() const noexcept { return gateEnvelopeGenerator.isGateOpen(); }
    float getGateEnvelope() const noexcept { return gateEnvelopeGenerator.getCurrentEnvelope(); }
    float getInputMeterLevel() const noexcept { return inputMeterLevel.load(std::memory_order_relaxed); }
    float getTriggerLevel() const noexcept { return triggerLevelDisplay.load(std::memory_order_relaxed); }

private:
    ProgramManager programManager;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* trigHPParam = nullptr;
    std::atomic<float>* trigLPParam = nullptr;
    std::atomic<float>* keySourceParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* holdParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* shapeParam = nullptr;
    std::atomic<float>* algorithmParam = nullptr;
    std::atomic<float>* sizeParam = nullptr;
    std::atomic<float>* preDelayParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* densityParam = nullptr;
    std::atomic<float>* dampHFParam = nullptr;
    std::atomic<float>* dampLFParam = nullptr;
    std::atomic<float>* slamParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* trimParam = nullptr;

    TriggerDetector triggerDetector;
    GateEnvelopeGenerator gateEnvelopeGenerator;
    juce::dsp::DelayLine<float> preDelayLine { 1 };
    ReverbEngine reverbEngine;
    DampingStage dampingStage;
    SlamSaturation slamSaturation;
    StereoWidthStage stereoWidthStage;
    OutputMixStage outputMixStage;

    juce::AudioBuffer<float> dryBuffer;
    std::vector<float> triggerLevelScratch;
    std::vector<float> gateGainScratch;

    std::atomic<float> inputMeterLevel { 0.0f };
    std::atomic<float> triggerLevelDisplay { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GatecrasherAudioProcessor)
};
