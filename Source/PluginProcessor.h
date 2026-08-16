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
#include <nf/UserEditGate.h>
#include <vector>

class GatecrasherAudioProcessor final : public juce::AudioProcessor
{
public:
    GatecrasherAudioProcessor();
    ~GatecrasherAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /** **Gatecrasher has no bypass, and that is a decision rather than an omission.**

        BRAND.md makes a disengaged state optional; the suite audit found four castings saying

        which they had chosen and two - this one and TapeRot - saying nothing either way, which

        leaves a reader unable to tell a decision from an oversight.

        The decision: this is a gate, and its whole subject is a signal being let through or not.

        A panel-level bypass would be a second, quieter answer to the question the GATE OPEN lamp

        already answers, and the host bypasses the plugin itself. Reflect-84 is the reference if

        that is ever reversed - a real AudioParameterBool behind getBypassParameter(), a 0.50

        multiply over the whole canvas, no caption, no control on the panel. */

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    // Rough estimate, not a hard bound: the reverb tanks are feedback networks whose actual decay
    // time varies continuously with the automatable Size/Decay parameters, up to a genuinely long
    // tail at their maxima (see Wall of Sound/Detonator) - this is a conservative value for hosts
    // that use it for freeze/bounce trailing-silence purposes, not a literal measured RT60.
    double getTailLengthSeconds() const override { return 10.0; }

    //==============================================================================
    /** **The host adapter - the ONLY place a Program is addressed by position.**

        Everything else identifies a Program by ProgramId; these four exist because the JUCE API is
        positional, and they translate at the boundary.

        **The list is the Factory bank and nothing else** - not INIT, not User Programs. That is a
        conformance requirement: juce_AudioProcessor.h documents getNumPrograms as "The value
        returned must be valid as soon as this object is created, and must not change over its
        lifetime", and a count including User Programs changed the moment one was saved.

        Before anyone makes the count dynamic again: JUCE's VST3 wrapper builds the automatable
        Program parameter ONCE in its constructor from this value, so a Program saved afterwards was
        unreachable from the host and a deleted one left the range overrunning the list. That was
        the API keeping its documented promise, not a bug to work around.

        Excluding INIT too means host index n IS Factory Program n+1.

        **Accepted divergence.** getCurrentProgram must answer with SOME factory position while a
        User Program is loaded, and answers 0 - so a host's menu shows a Factory name while the
        panel shows the user's Program. Sound and panel are both correct; only the host's own menu
        is wrong, and that is the format's limitation. */
    int getNumPrograms() override { return programManager.getNumPrograms(); }
    int getCurrentProgram() override { return programManager.getCurrentFactoryPosition(); }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override { return programManager.getProgramName(index); }
    /** Deliberately a no-op, and this comment is the point of it: with Factory-only exposure there
        is nothing on the host's list that can be renamed - Factory names are fixed and User
        Programs are not exposed. Implementing it would be a back door into the Factory bank, which
        is what the permanent slugs exist to prevent. */
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Factory programs (indices [0, kNumFactoryPrograms)) are the read-only, always-present
    // entries in kFactoryPrograms; user programs (indices [kNumFactoryPrograms, getNumPrograms()))
    // are files in ProgramManager's program directory. "Save" is never in-place for a factory
    // program, and never overwrites an existing user program either - it always creates a new one.
    /** The Program model. The panel talks to this directly, in ProgramIds - the four positional
        overrides above are the host's boundary and nothing else crosses it. */
    ProgramManager& getProgramManager() noexcept { return programManager; }

    bool isCurrentProgramModified() const { return programManager.isModifiedFromLoadedProgram(); }
    void saveNewUserProgram(const juce::String& name) { programManager.saveNewUserProgram(name); }
    void deleteUserProgram(const ProgramId& id) { programManager.deleteUserProgram(id); }

    /** **Guards a host replaying a stale program index over a just-restored session.** Armed by
        setStateInformation, consumed by the next setCurrentProgram (which ignores it only when the
        index matches what getCurrentProgram already reports — the shape of a replay), disarmed by
        the first USER-originated edit. **Automation must not disarm it**: a host may write
        automation on load before replaying, and that would reopen the hole.

        Public because the editor hands it to `nf::connectUserEdit` for every control, which is the
        point of it living in core: Reflect-84 once shipped this guard with zero call sites for its
        disarm, and coupling the disarm to the LCD hand-off is what makes that omission
        inexpressible. See nf/UserEditGate.h. */
    nf::UserEditGate userEdits;

    juce::AudioProcessorValueTreeState apvts;

    // GUI-facing derived-display state, mirroring TapeRot's poll-based pattern. The scope/meter
    // components poll these once per repaint and accumulate their own scrolling history locally,
    // rather than the audio thread maintaining a shared history ring buffer - simpler, and
    // sufficient since the GUI redraws far more often than the display needs to be exact.
    bool isGateOpen() const noexcept { return gateEnvelopeGenerator.isGateOpen(); }
    float getGateEnvelope() const noexcept { return gateEnvelopeGenerator.getCurrentEnvelope(); }
    float getInputMeterLevel() const noexcept { return inputMeterLevel.load(std::memory_order_relaxed); }
    float getOutputMeterLevel() const noexcept { return outputMeterLevel.load(std::memory_order_relaxed); }
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
    /*  The panel's ALGORITHM choice order is the SPEC's (GUI-SPEC 9.1) and `ReverbAlgorithm`'s is
        the DSP's own; they are unrelated, and a `static_cast` between them compiles and silently
        runs the wrong tank. One function so `prepare` and `processBlock` cannot disagree about the
        correspondence — they used to be the only two callers and only one of them had a map. */
    static constexpr ReverbAlgorithm tankForAlgorithmChoice (int choiceIndex) noexcept
    {
        constexpr ReverbAlgorithm byChoice[4] = {
            ReverbAlgorithm::ambience, ReverbAlgorithm::room,
            ReverbAlgorithm::plate,    ReverbAlgorithm::chamber };

        return byChoice[choiceIndex < 0 ? 0 : (choiceIndex > 3 ? 3 : choiceIndex)];
    }

    ReverbEngine reverbEngine;
    DampingStage dampingStage;
    SlamSaturation slamSaturation;
    StereoWidthStage stereoWidthStage;
    OutputMixStage outputMixStage;

    juce::AudioBuffer<float> dryBuffer;
    std::vector<float> triggerLevelScratch;
    std::vector<float> gateGainScratch;

    std::atomic<float> inputMeterLevel { 0.0f };
    std::atomic<float> outputMeterLevel { 0.0f };
    std::atomic<float> triggerLevelDisplay { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GatecrasherAudioProcessor)
};
