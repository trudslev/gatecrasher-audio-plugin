#include "TestUtils.h"

#include <set>
#include "../Source/DSP/ProgramManager.h"
#include "../Source/Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
    class DummyProcessor final : public juce::AudioProcessor
    {
    public:
        const juce::String getName() const override { return "Dummy"; }
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override { return 0.0; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}
        void getStateInformation(juce::MemoryBlock&) override {}
        void setStateInformation(const void*, int) override {}
    };
}

// Deliberately does not exercise initialise()/saveNewUserProgram()/deleteUserProgram(): those touch
// the real per-OS user Presets/Programs directory, which an automated test suite shouldn't write to
// or depend on the contents of (mirrors TapeRot's own restraint - FactoryPresetsTests.cpp checks the
// in-memory factory table structurally and nothing else exercises real preset file I/O). The async
// requestProgramChange -> apply path also isn't covered here since it needs a running
// juce::MessageManager dispatch loop to actually fire; that path is exercised manually per
// BUILDING.md instead.
class ProgramManagerTests final : public juce::UnitTest
{
public:
    ProgramManagerTests() : juce::UnitTest("ProgramManager", "DSP") {}

    void runTest() override
    {
        beginTest("Factory slugs are unique, non-empty and pinned as literals");
        {
            std::set<juce::String> slugs;

            for (const auto& fp : kFactoryPrograms)
            {
                const juce::String slug { fp.slug };
                expect(slug.isNotEmpty(), juce::String(fp.name) + " has no slug");
                expect(slugs.insert(slug).second, "duplicate slug: " + slug);
                expect(! slug.containsChar(' '), "slug must be filename- and XML-safe: " + slug);
            }

            expectEquals((int) slugs.size(), (int) kFactoryPrograms.size());

            // **Literals on purpose.** The display name above a slug may be revised freely; the slug
            // may not, because it is what a saved session stores. Asserted through the struct this
            // would follow a rename silently and prove nothing.
            expect(juce::String(kFactoryPrograms[0].slug) == "air-tomorrow");
            expect(juce::String(kInitProgram.slug) == "init");
        }

        beginTest("Every factory position round-trips through identity");
        {
            DummyProcessor proc;
            juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            ProgramManager manager(apvts);

            for (int i = 0; i < kNumFactoryPrograms; ++i)
            {
                const auto id = ProgramManager::factoryIdAt(i);
                expectEquals(ProgramManager::factoryPositionOf(id.id), i);
                expectEquals(manager.getProgramName(i), juce::String(kFactoryPrograms[(size_t) i].name));

                // Host index n IS Factory Program n+1 - the alignment excluding INIT buys.
                expect(manager.displayLabelFor(id).startsWith(juce::String(i + 1).paddedLeft('0', 2)));
            }
        }

        beginTest("The host list is the Factory bank, and INIT is not on it");
        {
            DummyProcessor proc;
            juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            ProgramManager manager(apvts);

            expectEquals(manager.getNumPrograms(), kNumFactoryPrograms);

            // juce_AudioProcessor.h: the value "must not change over its lifetime". The bank is a
            // compile-time array, so the only thing that could have changed it was counting user
            // files - which this no longer does.
            expect(manager.getProgramName(kNumFactoryPrograms).isEmpty(),
                   "nothing beyond the Factory bank may be addressable by position");
        }

        beginTest("The current Program defaults to the default Factory Program");
        {
            DummyProcessor proc;
            juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            ProgramManager manager(apvts);

            expect(manager.getCurrentProgramId() == ProgramManager::factoryIdAt(defaultFactoryProgramIndex));
            expectEquals(manager.getCurrentFactoryPosition(), defaultFactoryProgramIndex);
        }

        beginTest("setCurrentProgramWithoutApplying moves the identity without touching the APVTS");
        {
            DummyProcessor proc;
            juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            ProgramManager manager(apvts);

            const float thresholdBefore = *apvts.getRawParameterValue(ParamIDs::threshold);
            manager.setCurrentProgramWithoutApplying(ProgramManager::factoryIdAt(5));

            expect(manager.getCurrentProgramId() == ProgramManager::factoryIdAt(5));
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::threshold), thresholdBefore, 1.0e-6f);
        }

        beginTest("An unresolved identifier keeps its name for display and resolves to nothing");
        {
            DummyProcessor proc;
            juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            ProgramManager manager(apvts);

            const auto id = manager.resolve(ProgramBank::factory, "a-program-from-the-future",
                                             "SOME FUTURE SOUND");

            expect(id.bank == ProgramBank::unresolved);
            expect(id.displayName == "SOME FUTURE SOUND",
                   "the panel needs a presentable name - a slug would read as a rendering fault");

            // And the label must not acquire a number: it is in no bank, so it has no position.
            expect(manager.displayLabelFor(id) == "SOME FUTURE SOUND");
        }
    }
};

static ProgramManagerTests programManagerTests;
