#include <juce_audio_processors/juce_audio_processors.h>
#include <nf/PrintedScale.h>
#include "../Source/GUI/GatecrasherTheme.h"
#include "../Source/PluginProcessor.h"

/*  **§3.2's rings, checked against the thing that drives the pointer.**

    BRAND.md makes a printed scale a correctness requirement, and the failure it guards is invisible:
    a ring legends a taper, so if the parameter's taper moves and the ring does not, the numerals
    point at values the pointer never reaches. The ring still draws, the pointer still turns, and
    nothing looks broken.

    **So the authority is each parameter's own `NormalisableRange`, never a second table of angles.**
    A test comparing stored angles with stored angles asserts that somebody transcribed a spec
    consistently and says nothing about whether the ring matches the control.

    This casting had **no printed-scale check at all** until 2026-08-20, which was correct while its
    ticks and numerals were pixels in a plate — there was no code table to check, and the suite's own
    table records that as a genuine "does not apply" rather than a gap. The plate is gone and the
    rings are `Layout::knobScales`, so the exemption went with it. Five of six castings already had
    one; TapeRot's lives in `RotaryArcTests.cpp` and Chorus-60's asserts through its theme.

    §12 lists confirming the four skewed rings against the build's ranges as outstanding, with the
    ruling that **where they disagree the build wins and the artwork is re-cut**. This is that
    confirmation, and it runs on every build rather than once.
*/
class PrintedScaleTests final : public juce::UnitTest
{
public:
    PrintedScaleTests() : juce::UnitTest ("Printed scales", "GUI") {}

    void runTest() override
    {
        using namespace GatecrasherTheme;

        GatecrasherAudioProcessor processor;

        beginTest ("Every ring legends its own parameter's taper");

        for (const auto& scale : Layout::knobScales)
        {
            // The selector has four detents and no numerals: §3.3 gives it corner labels, which are
            // printed ink rather than a scale, so there is nothing here to legend a taper with.
            if (juce::String (scale.paramID) == "algorithm")
                continue;

            auto* raw = processor.apvts.getParameter (scale.paramID);
            expect (raw != nullptr, juce::String (scale.paramID) + " has no parameter");
            if (raw == nullptr)
                continue;

            auto* asFloat = dynamic_cast<juce::AudioParameterFloat*> (raw);
            expect (asFloat != nullptr, juce::String (scale.paramID) + " is not a float parameter");
            if (asFloat == nullptr)
                continue;

            /*  **Every marked position goes in, numbered or not.** What a standard-class knob drops
                is the numeral, and a minor at a real value is exactly as capable of pointing at the
                wrong place as a numeral is — checking only the printed ones would leave two thirds
                of HP's and LP's rings unverified.  */
            std::vector<nf::PrintedMark> marks;
            for (int i = 0; i < scale.markCount; ++i)
            {
                const auto& mark = scale.marks[(size_t) i];
                const juce::String text (juce::CharPointer_UTF8 (mark.numeral));
                if (text.isEmpty())
                    continue;

                marks.push_back ({ parseNumeral (text), mark.angleDeg });
            }

            const auto defects = nf::printedScaleDefects (asFloat->getNormalisableRange(), marks);
            expect (defects.isEmpty(),
                    juce::String (scale.paramID) + ": " + defects.joinIntoString ("; ")
                        + (juce::String (scale.paramID) == "hold"
                               ? juce::String ("\n  OPEN FINDING, DELIBERATELY RED — HOLD's ring is "
                                 "evenly spaced and its parameter is not. §12's own ruling applies "
                                 "(the build wins, the artwork is re-cut) and the figures are "
                                 "Design's to give, so it is raised as "
                                 "design-asks/gatecrasher-hold-ring-is-evenly-spaced.md rather than "
                                 "applied. NOT a regression, and NOT a bug in this check — the four "
                                 "SKEWED rings all reproduce their tapers. Passing it would mean "
                                 "widening the tolerance past 4.13 degrees, which waives the one "
                                 "property this check asserts.")
                               : juce::String()));
        }

        /*  **The arm that proves the one above can fail.** A printed-scale check that only ever
            passes is indistinguishable from one that cannot, and this suite has three recorded cases
            of exactly that. HP is the case to break: it is skewed 0.3, so its middle mark sits at
            −3.51° where even spacing would put it at 0, and an evenly-spaced ring is precisely what
            compiles, runs and looks plausible.  */
        beginTest ("An evenly-spaced ring on a skewed parameter is caught");
        {
            auto* hp = dynamic_cast<juce::AudioParameterFloat*> (processor.apvts.getParameter ("trigHP"));
            expect (hp != nullptr);

            if (hp != nullptr)
            {
                const std::vector<nf::PrintedMark> evenlySpaced {
                    { 20.0f, -135.0f }, { 50.0f, -67.5f }, { 200.0f, 0.0f },
                    { 800.0f, 67.5f }, { 2000.0f, 135.0f } };

                const auto defects = nf::printedScaleDefects (hp->getNormalisableRange(), evenlySpaced);
                expect (! defects.isEmpty(),
                        "an evenly-spaced ring on a 0.3-skewed range must be caught, and was not");
                logMessage ("  even spacing on HP -> " + defects.joinIntoString ("; "));
            }
        }

        /*  §3.1's numeral counts, which the mark table encodes and nothing else states: five on the
            Ø76 primaries and three on the Ø56 standards, with the minors holding the rest at their
            real values.  */
        beginTest ("§3.1's numeral counts — five on primary, three on standard");

        for (const auto& spec : Layout::knobs)
        {
            const auto* scale = std::find_if (Layout::knobScales.begin(), Layout::knobScales.end(),
                                               [&] (const auto& s) { return juce::String (s.paramID) == spec.paramID; });
            expect (scale != Layout::knobScales.end(), juce::String (spec.paramID) + " has no scale");
            if (scale == Layout::knobScales.end() || juce::String (spec.paramID) == "algorithm")
                continue;

            int numbered = 0;
            for (int i = 0; i < scale->markCount; ++i)
                if (scale->marks[(size_t) i].numbered
                    && juce::String (juce::CharPointer_UTF8 (scale->marks[(size_t) i].numeral)).isNotEmpty())
                    ++numbered;

            // TRIM is the one four-mark ring: §3.2 gives it −24 · −12 · 0 · +12, three numbered.
            const int expected = spec.diameter > 60.0f ? 5 : 3;
            expect (numbered == expected,
                    juce::String (spec.paramID) + " prints " + juce::String (numbered)
                        + " numerals, §3.1 says " + juce::String (expected));
        }

        /*  **§8's UNIT row, which had no drawing site at all until 2026-08-22.**

            Fifteen constants existed and were correct — `knobScales`' per-knob `unit`,
            `knobUnitCssPx` / `knobUnitLineBox` / `knobUnitTrackingEm`, and §3.1's `knobUnitTop(d)`
            registration — and **nothing in `Source/` consumed any of them.** A spec role with a
            complete, correct, unused implementation reads exactly like a finished one.

            Eleven of fifteen rings print a unit and four do not, and that split is not a judgement:
            the four silent ones are the four with nothing to print — ALGORITHM is a detented
            selector with no numerals, and SIZE, HF and LF are unitless 0–1.0 controls. So §8's row
            needed implementing rather than qualifying.

            **The check is against the PARAMETER, which is outside the table.** Comparing the table's
            unit with the string the panel draws would agree with itself. Two rings are exempt by
            name and with the reason: Trigger HP and LP switch between Hz and kHz at 1000, so their
            unit is value-dependent and `hzAttrs` deliberately carries no label — the same shape as
            Elmer's SIDECHAIN HP, and the same reason a blanket assertion would have been wrong.  */
        beginTest ("Every ring's printed unit is its parameter's own label");
        {
            int checked = 0, valueDependent = 0, unitless = 0;

            for (const auto& scale : Layout::knobScales)
            {
                auto* param = dynamic_cast<juce::RangedAudioParameter*> (
                    processor.apvts.getParameter (scale.paramID));

                if (param == nullptr)
                    continue;

                const juce::String tableUnit (scale.unit != nullptr ? scale.unit : "");
                const juce::String paramLabel = param->getLabel();
                const juce::String id (scale.paramID);

                if (id == "trigHP" || id == "trigLP")
                {
                    ++valueDependent;
                    expect (tableUnit == "Hz",
                            id + " is the value-dependent pair and its ring prints Hz");
                    expect (paramLabel.isEmpty(),
                            id + " must carry no label — its unit moves to kHz above 1000, which is "
                                 "why the unit lives in the value text");
                    continue;
                }

                if (tableUnit.isEmpty())
                {
                    ++unitless;
                    expect (paramLabel.isEmpty(),
                            id + " prints no unit but its parameter carries the label '"
                                + paramLabel + "' — one of the two is wrong");
                    continue;
                }

                ++checked;
                expectEquals (tableUnit, paramLabel,
                              id + "'s printed unit is not its parameter's label");
            }

            logMessage ("  " + juce::String (checked) + " rings checked against their label, "
                            + juce::String (valueDependent) + " value-dependent, "
                            + juce::String (unitless) + " unitless");

            expectEquals (checked + valueDependent, 11,
                          "§8's unit row applies to eleven rings; the count has moved");
            expectEquals (unitless, 4,
                          "four rings have nothing to print — ALGORITHM, SIZE, HF, LF");
        }
    }

private:
    /** §3.2 prints `2k`, `20k` and a U+2212 minus, so the numeral is not a bare `getFloatValue()`. */
    static float parseNumeral (juce::String text)
    {
        text = text.replace (juce::String::charToString ((juce::juce_wchar) 0x2212), "-");

        if (text.endsWithIgnoreCase ("k"))
            return text.dropLastCharacters (1).getFloatValue() * 1000.0f;

        return text.getFloatValue();
    }
};

static PrintedScaleTests printedScaleTests;
