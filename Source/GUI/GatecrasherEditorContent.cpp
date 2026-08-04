#include "GatecrasherEditorContent.h"
#include "../Parameters.h"

using namespace GatecrasherTheme;

namespace
{
    const char* knobTooltip(const juce::String& paramID)
    {
        if (paramID == ParamIDs::threshold) return "Level the input/sidechain trigger must cross to open the gate.";
        if (paramID == ParamIDs::trigHP)    return "High-pass filter on the trigger-detection path only - never touches the audio signal.";
        if (paramID == ParamIDs::trigLP)    return "Low-pass filter on the trigger-detection path only - never touches the audio signal.";
        if (paramID == ParamIDs::attack)    return "How fast the gate opens once triggered.";
        if (paramID == ParamIDs::hold)      return "How long the gate stays fully open after the last trigger before releasing.";
        if (paramID == ParamIDs::release)   return "How fast the gate closes once Hold elapses - see the SHAPE switch for its curve.";
        if (paramID == ParamIDs::algorithm) return "Reverb tank algorithm: Room, Plate, Chamber, or Ambience.";
        if (paramID == ParamIDs::size)      return "Reverb tank size.";
        if (paramID == ParamIDs::preDelay)  return "Delay before the reverb tank's input, ahead of the gate.";
        if (paramID == ParamIDs::dampHF)    return "High-frequency damping inside the reverb tank.";
        if (paramID == ParamIDs::dampLF)    return "Low-frequency damping inside the reverb tank.";
        if (paramID == ParamIDs::slam)      return "Drive into the tank's saturation stage.";
        if (paramID == ParamIDs::width)     return "Stereo width of the processed signal.";
        if (paramID == ParamIDs::mix)       return "Dry/wet blend between the unprocessed and processed signal.";
        if (paramID == ParamIDs::trim)      return "Output level trim, applied after the dry/wet mix.";
        return nullptr;
    }

    // Knob drag-value popup text. Mirrors TapeRotEditorContent's own formatKnobPopupText:
    // AudioParameterFloatAttributes::withLabel() only feeds getLabel(), not getText(), so a
    // continuous float parameter's default getText() would otherwise render unlabelled,
    // many-decimal-place values while dragging. algorithm (the only choice param with a knob) has
    // no label and formats fine through getText() already, so it's left alone.
    juce::String formatKnobPopupText(const juce::RangedAudioParameter& param, double value)
    {
        const auto label = param.getLabel();
        if (label.isEmpty())
            return param.getText(param.convertTo0to1((float) value), 0);

        int decimalPlaces = 1;
        if (label == "Hz")
            decimalPlaces = 0;

        const juce::String text(value, decimalPlaces);
        return label == "%" ? text + label : text + " " + label;
    }
}

GatecrasherEditorContent::GatecrasherEditorContent(GatecrasherAudioProcessor& p)
    : processorRef(p), gateScope(p), gateLamp(p), inputMeter(p), programHeader(p)
{
    setSize((int) Layout::canvasWidth, (int) Layout::canvasHeight);
    setLookAndFeel(&lookAndFeel);

    panelBackground.setBounds(getLocalBounds());
    addAndMakeVisible(panelBackground);

    wordmark.setBounds(getLocalBounds());
    addAndMakeVisible(wordmark);

    for (size_t i = 0; i < Layout::knobs.size(); ++i)
    {
        const auto& spec = Layout::knobs[i];
        const float tickSpacing = spec.size == KnobFilmstripSize::large
                                       ? Layout::largeKnobTickSpacingDegrees
                                       : Layout::smallKnobTickSpacingDegrees;

        auto knob = std::make_unique<KnobFilmstripComponent>(spec.size, spec.diameter, tickSpacing,
                                                               spec.isAlgorithmSelector);
        knob->setName(spec.paramID);

        // Bounding box reaches the tick ring's outer radius (+3px click margin), not just the
        // knob's own diameter - matches TapeRot's `knobTickOuterRadius + 3` convention.
        const float half = spec.diameter * 0.5f + Layout::tickOuterOffset + 3.0f;
        knob->setBounds((int) std::round(spec.cx - half), (int) std::round(spec.cy - half),
                         (int) std::round(half * 2.0f), (int) std::round(half * 2.0f));

        if (const auto* tooltip = knobTooltip(spec.paramID))
            knob->setTooltip(tooltip);

        knobAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.apvts, spec.paramID, *knob);

        // Live value readout while dragging, parented to this (not nullptr) so PluginEditor's
        // uniform scale transform applies to the popup too, same as every other on-canvas element.
        if (auto* param = processorRef.apvts.getParameter(spec.paramID))
        {
            knob->textFromValueFunction = [param](double value) { return formatKnobPopupText(*param, value); };
            knob->setPopupDisplayEnabled(true, false, this);
        }

        addAndMakeVisible(*knob);
        knobs[i] = std::move(knob);
    }

    auto placeSwitch = [](ToggleSwitchComponent& sw, float trackX, float trackY)
    {
        sw.setBounds((int) std::round(trackX - Layout::switchAssemblyPad),
                     (int) std::round(trackY - Layout::switchCaptionRowH),
                     (int) std::round(Layout::switchAssemblyW), (int) std::round(Layout::switchAssemblyH));
    };

    placeSwitch(keySourceSwitch, Layout::keySourceTrackX, Layout::keySourceTrackY);
    keySourceSwitch.setTooltip("Trigger detector source: Internal (main input) or Sidechain.");
    keySourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, ParamIDs::keySource, keySourceSwitch);
    addAndMakeVisible(keySourceSwitch);

    placeSwitch(shapeSwitch, Layout::shapeTrackX, Layout::shapeTrackY);
    shapeSwitch.setTooltip("Release curve character: Hard is a linear cliff, Soft curves and extends it.");
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, ParamIDs::shape, shapeSwitch);
    addAndMakeVisible(shapeSwitch);

    // GateScope/GateLamp/InputMeter/ProgramHeader draw with absolute canvas coordinates (like
    // GatecrasherPanelBackground/WordmarkComponent), so they're sized to the full canvas rather
    // than a sub-region - each narrows its own hitTest (ProgramHeader) or opts out of mouse input
    // entirely (the pure-display ones) so they don't swallow clicks meant for the knobs/switches.
    gateScope.setBounds(getLocalBounds());
    addAndMakeVisible(gateScope);

    gateLamp.setBounds(getLocalBounds());
    addAndMakeVisible(gateLamp);

    inputMeter.setBounds(getLocalBounds());
    addAndMakeVisible(inputMeter);

    programHeader.setBounds(getLocalBounds());
    addAndMakeVisible(programHeader);
}

GatecrasherEditorContent::~GatecrasherEditorContent()
{
    setLookAndFeel(nullptr);
}
