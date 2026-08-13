#pragma once

#include "DSP/FactoryPrograms.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    constexpr auto threshold = "threshold";
    constexpr auto trigHP = "trigHP";
    constexpr auto trigLP = "trigLP";
    constexpr auto keySource = "keySource";
    constexpr auto attack = "attack";
    constexpr auto hold = "hold";
    constexpr auto release = "release";
    constexpr auto shape = "shape";
    constexpr auto algorithm = "algorithm";
    constexpr auto size = "size";
    constexpr auto preDelay = "preDelay";
    constexpr auto decay = "decay";
    constexpr auto density = "density";
    constexpr auto dampHF = "dampHF";
    constexpr auto dampLF = "dampLF";
    constexpr auto slam = "slam";
    constexpr auto width = "width";
    constexpr auto mix = "mix";
    constexpr auto trim = "trim";
}

namespace KeySourceNames
{
    constexpr auto internalSource = "INTERNAL";
    constexpr auto sidechain = "SIDECHAIN";
}

namespace ShapeNames
{
    constexpr auto hard = "HARD";
    constexpr auto soft = "SOFT";
}

namespace AlgorithmNames
{
    // Declared in PANEL ORDER, which spec section 9.1 makes part of the contract: the selector's
    // four detents sweep AMBI -> ROOM -> PLATE -> CHMBR across -135 / -45 / +45 / +135, so index 0
    // is Ambience and the default Plate sits at index 2 (+45 degrees).
    constexpr auto ambience = "AMBIENCE";
    constexpr auto room = "ROOM";
    constexpr auto plate = "PLATE";
    constexpr auto chamber = "CHAMBER";
}

namespace LegacyMigration
{
    // Bumped whenever a stored parameter's *meaning* (not just its ID) changes incompatibly.
    // Written into getStateInformation's XML root; setStateInformation checks it and remaps legacy
    // values before restoring.
    constexpr auto stateSchemaVersionAttribute = "gatecrasherStateSchemaVersion";
    constexpr int currentStateSchemaVersion = 3;

    /** The schema at which the session stopped storing a positional index and started storing bank
        + identifier. Sessions at or above this carry the three attributes below; older ones carry
        "gatecrasherCurrentProgramIndex" and are mapped through the current bank on restore. */
    constexpr int identitySchemaVersion = 3;

    /** **The identity attributes, and they are a contract.** Rename one and the session still
        parses while the Program silently reverts, with no error anywhere.

        `...ProgramName` is DISPLAY ONLY - it exists so an unresolved identifier can still be named
        on the panel, since a factory slug is not presentable. It never resolves anything. */
    constexpr auto programBankAttribute = "gatecrasherProgramBank";
    constexpr auto programIdAttribute   = "gatecrasherProgramId";
    constexpr auto programNameAttribute = "gatecrasherProgramName";

    inline juce::String bankAttributeValue (ProgramBank bank)
    {
        switch (bank)
        {
            case ProgramBank::init:       return "init";
            case ProgramBank::factory:    return "factory";
            case ProgramBank::user:       return "user";
            case ProgramBank::unresolved: return "unresolved";
        }

        return "factory";
    }

    inline ProgramBank bankFromAttribute (const juce::String& value)
    {
        if (value == "init")       return ProgramBank::init;
        if (value == "user")       return ProgramBank::user;
        if (value == "unresolved") return ProgramBank::unresolved;

        return ProgramBank::factory;
    }

    // v1 -> v2: the Algorithm choice list was reordered to match the panel. It used to be declared
    // Room, Plate, Chamber, Ambience, which put all four labels on the wrong detent and rendered
    // the default where the plate prints ROOM; spec section 9.1 now fixes the order as part of the
    // contract. The index is what gets serialised, so every session, user Program and automation
    // lane written before this carries the old meaning and must be remapped.
    //
    //   old 0 Room     -> new 1
    //   old 1 Plate    -> new 2
    //   old 2 Chamber  -> new 3
    //   old 3 Ambience -> new 0
    inline constexpr std::array<int, 4> legacyAlgorithmRemapV1ToV2{ { 1, 2, 3, 0 } };

    /** Free function rather than a processor method so it is unit-testable against a synthetic
        XmlElement, without needing the plugin target's JucePlugin_* macros - the same shape as
        TapeRot's remapLegacyModelIndexIfNeeded. */
    /** **Gated on the version this migration belongs to, NOT on the current one.**

        It read `>= currentStateSchemaVersion` and worked only for as long as the schema stayed at
        2. Bumping it to 3 for the identity change immediately re-armed this hop for schema-2
        sessions, rotating the algorithm a second time on every load - a session saved on Plate
        reopening on Chamber, silently. The v1->v2 remap applies to v1 sessions and to nothing else,
        and now says so. */
    inline void remapLegacyAlgorithmIfNeeded(juce::XmlElement& xml)
    {
        constexpr int algorithmRemapAppliesBelow = 2;

        if (xml.getIntAttribute(stateSchemaVersionAttribute, 1) >= algorithmRemapAppliesBelow)
            return;

        for (int i = 0; i < xml.getNumChildElements(); ++i)
        {
            auto* child = xml.getChildElement(i);
            if (child != nullptr && child->getStringAttribute("id") == ParamIDs::algorithm)
            {
                const int oldIndex = (int) child->getDoubleAttribute("value");
                child->setAttribute("value", (double) legacyAlgorithmRemapV1ToV2[
                    (size_t) juce::jlimit(0, 3, oldIndex)]);
                break;
            }
        }
    }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createGatecrasherParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // withLabel() only feeds getLabel(); it does NOT change getText(), and an AudioParameterFloat
    // with no stringFromValue of its own renders through juce::String(float) at full precision. Left
    // at the default, section 6.3's "THRESHOLD: -18.5 dB" came out as "THRESHOLD: -16.3999977 dB" -
    // in the LCD, in the host's automation lane and in any generic editor. So every float parameter
    // gets an explicit formatter, and both the LCD and the host read the same string.
    //
    // The unit itself stays in the label rather than the text: the LCD joins the two with a space
    // (section 6.3) and JUCE's own generic UI appends the label the same way, so baking it into the
    // text would double it up.
    auto fixed = [] (int decimals)
    {
        return [decimals] (float v, int) { return juce::String(v, decimals); };
    };

    // **Whole numbers must NOT go through fixed(0).** juce::String(double, int) only sets a
    // formatting flag when the decimal count is greater than zero (juce_String.cpp:486-492); at
    // exactly 0 it sets nothing and falls through to std::ostream's default, which is six
    // significant digits with trailing zeros stripped. So fixed(0) does not round - it renders
    // 33.333332 as "33.3333" and 66.666664 as "66.6667", and only looks correct while the value
    // happens to be integral. That is the same class of defect as leaving the formatter off
    // entirely, which the comment above already warns about; 0 decimals reads like "no decimals"
    // and means "no formatting".
    auto whole = [] (float v, int) { return juce::String(juce::roundToInt(v)); };

    auto dbAttrs = juce::AudioParameterFloatAttributes().withLabel("dB")
                       .withStringFromValueFunction(fixed(1));
    auto msAttrs = juce::AudioParameterFloatAttributes().withLabel("ms")
                       .withStringFromValueFunction(fixed(1));
    auto percentAttrs = juce::AudioParameterFloatAttributes().withLabel("%")
                            .withStringFromValueFunction(whole);

    // Frequencies switch to kHz at 1000, matching both the printed scales ("1k", "5k", "20k") and
    // section 6.3's own "TRIG LP: 6.3 kHz" example. The label has to move into the text here because
    // the unit is value-dependent, so hzAttrs carries no label of its own.
    auto hzAttrs = juce::AudioParameterFloatAttributes()
                       .withStringFromValueFunction([] (float v, int)
                       {
                           // roundToInt below 1kHz for the same reason percentAttrs uses it: the
                           // 0-decimal form is not a rounding instruction. Trigger HP sweeps
                           // 20-2000 Hz with no interval, so it sits on a non-integral value far
                           // more often than not, and printed "180.437 Hz".
                           return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz"
                                                : juce::String(juce::roundToInt(v)) + " Hz";
                       });

    // Size / Decay / Density / Damping are bare 0-1 normals with no unit of their own; the panel
    // prints their scales as 0-1.0, so they read the same way here.
    auto normAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(fixed(2));

    // Gate section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::threshold, 1}, "THRESHOLD",
        juce::NormalisableRange<float>(-60.0f, 0.0f), -18.5f, dbAttrs));

    // Trigger-detection filters: log skew (0.3, matching TapeRot's LP/HP convention) so the
    // musically-relevant low end of the range isn't crammed into a sliver of knob travel.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::trigHP, 1}, "TRIGGER HP",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 0.0f, 0.3f), 180.0f, hzAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::trigLP, 1}, "TRIGGER LP",
        juce::NormalisableRange<float>(500.0f, 20000.0f, 0.0f, 0.3f), 6300.0f, hzAttrs));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamIDs::keySource, 1}, "KEY SOURCE",
        juce::StringArray{KeySourceNames::internalSource, KeySourceNames::sidechain}, 0));

    // Attack skews hard toward the low end (0.25) - sub-millisecond differences matter here and
    // the usable range tops out well before 20ms.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::attack, 1}, "ATTACK",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.0f, 0.25f), 0.4f, msAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::hold, 1}, "HOLD",
        juce::NormalisableRange<float>(10.0f, 500.0f), 165.0f, msAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::release, 1}, "RELEASE",
        juce::NormalisableRange<float>(1.0f, 200.0f, 0.0f, 0.3f), 4.0f, msAttrs));

    // Release-curve character only (see GateEnvelopeGenerator) - attack/hold are unaffected.
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamIDs::shape, 1}, "SHAPE",
        juce::StringArray{ShapeNames::hard, ShapeNames::soft}, 0));

    // Reverb section
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamIDs::algorithm, 1}, "ALGORITHM",
        juce::StringArray{AlgorithmNames::ambience, AlgorithmNames::room,
                           AlgorithmNames::plate, AlgorithmNames::chamber},
        2));   // default Plate, at +45 degrees - section 9.1

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::size, 1}, "SIZE",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.72f, normAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::preDelay, 1}, "PRE-DELAY",
        juce::NormalisableRange<float>(0.0f, 120.0f), 18.0f, msAttrs));

    // Decay: no panel control - approved design deliberately leaves this automation-only, same
    // treatment as Density below. Normalised 0-1 like Size/Density rather than a raw RT60 in
    // seconds, since it feeds each ReverbTank's own algorithm-specific feedback-coefficient curve
    // rather than a single shared time constant.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::decay, 1}, "DECAY",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f, normAttrs));

    // Density: no panel control by design - automation-only (GUI-SPEC.md §9).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::density, 1}, "DENSITY",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f, normAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::dampHF, 1}, "DAMPING HF",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.55f, normAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::dampLF, 1}, "DAMPING LF",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.35f, normAttrs));

    // Output section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::slam, 1}, "SLAM",
        juce::NormalisableRange<float>(0.0f, 12.0f), 7.0f, dbAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::width, 1}, "STEREO WIDTH",
        juce::NormalisableRange<float>(0.0f, 200.0f), 128.0f, percentAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::mix, 1}, "MIX",
        juce::NormalisableRange<float>(0.0f, 100.0f), 64.0f, percentAttrs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::trim, 1}, "OUTPUT TRIM",
        juce::NormalisableRange<float>(-24.0f, 12.0f), -1.4f, dbAttrs));

    // New parameters are appended below this line, never inserted above, to keep existing
    // sessions' and programs' parameter IDs stable.

    return {params.begin(), params.end()};
}
