#include "TestUtils.h"
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

/**
    What each parameter actually RENDERS - the string, not the value.

    This exists because a formatting bug is invisible to every other kind of test. Stereo Width and
    Mix carried `withStringFromValueFunction(fixed(0))`, which reads like "no decimal places" and
    is not: juce::String(double, int) only sets a formatting flag when the count is GREATER than
    zero (juce_String.cpp:486-492), so at exactly 0 it falls through to std::ostream's default of
    six significant digits. The LCD showed `MIX: 33.3333 %`. Every value assertion in the suite
    still passed, because the value was right and only its rendering was wrong.

    **The expected strings are literals.** Deriving them from the same formatter the code uses
    would make this test rename itself alongside a regression and assert nothing - the same trap
    that let a migration-guard bug survive its own test elsewhere in this suite.

    The values chosen are deliberately non-integral. An integral value renders identically under
    the broken and the fixed formatter, so a test written against 50.0 would have passed throughout.
*/
class ParameterTextTests final : public juce::UnitTest
{
public:
    ParameterTextTests() : juce::UnitTest("Parameter text", "DSP") {}

    void runTest() override
    {
        DummyProcessor proc;
        juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMETERS",
                                                  createGatecrasherParameterLayout());

        // Sets a parameter to a real-world value and returns what the LCD and the host would show.
        const auto textAt = [&apvts] (const juce::String& id, float value)
        {
            auto* p = apvts.getParameter(id);
            p->setValueNotifyingHost(p->convertTo0to1(value));
            return p->getText(p->getValue(), 0);
        };

        beginTest("Percentages are whole numbers at every value, not just integral ones");
        expectEquals(textAt(ParamIDs::mix, 33.333332f), juce::String("33"));
        expectEquals(textAt(ParamIDs::mix, 66.666664f), juce::String("67"));
        expectEquals(textAt(ParamIDs::mix, 99.9f), juce::String("100"));
        expectEquals(textAt(ParamIDs::mix, 50.0f), juce::String("50"));
        expectEquals(textAt(ParamIDs::width, 128.4f), juce::String("128"));

        beginTest("Frequencies below 1 kHz are whole Hz");
        expectEquals(textAt(ParamIDs::trigHP, 180.437f), juce::String("180 Hz"));
        expectEquals(textAt(ParamIDs::trigHP, 20.6f), juce::String("21 Hz"));

        beginTest("Frequencies at or above 1 kHz switch to kHz at one decimal");
        expectEquals(textAt(ParamIDs::trigLP, 6300.0f), juce::String("6.3 kHz"));
        expectEquals(textAt(ParamIDs::trigLP, 19999.0f), juce::String("20.0 kHz"));

        beginTest("dB and ms keep exactly one decimal");
        expectEquals(textAt(ParamIDs::threshold, -18.5f), juce::String("-18.5"));
        expectEquals(textAt(ParamIDs::threshold, -16.3999977f), juce::String("-16.4"));

        beginTest("No numeric parameter ever renders more than two decimal places");
        // The general form of the defect, rather than a length limit. A character cap does not
        // catch it: the broken percent formatter rendered "33.3333", which is only seven
        // characters and would fit any budget the LCD actually has. What is always wrong is the
        // DECIMAL COUNT, because std::ostream's default gives six significant digits and every
        // formatter here is specified at two places or fewer.
        //
        // Two, not one: Size / Decay / Density / Damping are bare 0-1 normals rendered by
        // `normAttrs` at fixed(2), because the panel prints their scales as 0-1.0 and one decimal
        // would leave only eleven distinguishable readings. Three or more is always a formatter
        // that has fallen through.
        //
        // Choice and bool parameters are excluded deliberately: they render a name ("Sidechain"),
        // not a number, and have no decimal count to check.
        for (auto* p : apvts.processor.getParameters())
        {
            if (dynamic_cast<juce::AudioParameterChoice*>(p) != nullptr
                || dynamic_cast<juce::AudioParameterBool*>(p) != nullptr)
                continue;

            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                for (float t : { 0.0f, 0.137f, 0.5f, 0.618f, 0.911f, 1.0f })
                {
                    const auto rendered = ranged->getText(t, 0);
                    const auto dot = rendered.indexOfChar('.');
                    if (dot < 0)
                        continue;

                    int decimals = 0;
                    for (int i = dot + 1; i < rendered.length()
                                          && juce::CharacterFunctions::isDigit(rendered[i]); ++i)
                        ++decimals;

                    expect(decimals <= 2,
                           ranged->getParameterID() + " renders \"" + rendered + "\" ("
                               + juce::String(decimals) + " decimals) at t=" + juce::String(t));
                }
        }
    }
};

static ParameterTextTests parameterTextTests;
