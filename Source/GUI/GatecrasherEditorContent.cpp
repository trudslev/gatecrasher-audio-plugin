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
}

GatecrasherEditorContent::GatecrasherEditorContent(GatecrasherAudioProcessor& p)
    : processorRef(p), stateLabels(p), gateScope(p), gateLamp(p), inputMeter(p), programHeader(p)
{
    setSize((int) Layout::canvasWidth, (int) Layout::canvasHeight);
    setLookAndFeel(&lookAndFeel);

    panelBackground.setBounds(getLocalBounds());
    addAndMakeVisible(panelBackground);

    // The plate carries every static label, numeral, tick and the wordmark (spec section 0.2), so
    // there is no engraved layer to draw and no wordmark component. The only fascia text this build
    // produces is the eight state-dependent labels below.
    stateLabels.setBounds(getLocalBounds());
    addAndMakeVisible(stateLabels);

    for (size_t i = 0; i < Layout::knobs.size(); ++i)
    {
        const auto& spec = Layout::knobs[i];

        auto knob = std::make_unique<KnobFilmstripComponent>(spec.size, spec.diameter);
        knob->setName(spec.paramID);

        // Bounds are the filmstrip's FRAME box - 1.333 x the cap, section 1.3 - because the whole
        // frame has to be blitted for the baked shadow to fade out inside it. The hit area is NOT
        // this: KnobFilmstripComponent::hitTest narrows it to the cap, so the transparent margin
        // lying over the plate's printed numerals does not swallow clicks meant for bare fascia.
        const float half = spec.diameter * Layout::knobBoundingBoxBleed * 0.5f;
        knob->setBounds((int) std::round(spec.cx - half), (int) std::round(spec.cy - half),
                         (int) std::round(half * 2.0f), (int) std::round(half * 2.0f));

        if (const auto* tooltip = knobTooltip(spec.paramID))
            knob->setTooltip(tooltip);

        knobAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.apvts, spec.paramID, *knob);

        // Section 6.3 puts the live value in the PROGRAM LCD rather than a drag popup, and section
        // 0.2 is explicit that no number appears on the fascia outside the two LCD windows and the
        // printed scales.
        //
        // Guarded on the control's OWN drag state. A SliderAttachment also fires when a program is
        // applied and on every host automation step; without the guard the LCD latches onto
        // whichever parameter was written last and flickers for the length of a song.
        auto* rawKnob = knob.get();
        const juce::String paramID(spec.paramID);
        // The same guard disarms the processor's stale-replay flag, because this is the only place
        // that knows a change came from a PERSON. It deliberately does not fire for automation: a
        // host may write automation on session load before replaying its remembered program index,
        // and disarming there would let that replay land on the restored state.
        knob->onValueChange = [this, rawKnob, paramID]
        {
            if (rawKnob->isMouseButtonDown())
            {
                processorRef.noteUserEdit();
                programHeader.showParameter(paramID);
            }
        };
        knob->onDragEnd = [this] { programHeader.releaseParameter(); };

        addAndMakeVisible(*knob);
        knobs[i] = std::move(knob);
    }

    // Widened by switchLabelOverflowPad on each side and switchVerticalSafetyPad top/bottom, beyond
    // the track's own spec'd footprint, so there's room to erase-then-redraw the whole assembly
    // (caption, track, option labels) in full - see those constants' comments. ToggleSwitchComponent
    // draws everything offset by the same pads, so the track's visual position matches
    // trackX/trackY exactly regardless of this extra margin.
    auto placeSwitch = [](ToggleSwitchComponent& sw, float trackX, float trackY)
    {
        sw.setBounds((int) std::round(trackX - Layout::switchAssemblyPad - Layout::switchLabelOverflowPad),
                     (int) std::round(trackY - Layout::switchCaptionRowH - Layout::switchVerticalSafetyPad),
                     (int) std::round(Layout::switchAssemblyW + 2.0f * Layout::switchLabelOverflowPad),
                     (int) std::round(Layout::switchAssemblyH + 2.0f * Layout::switchVerticalSafetyPad));
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

    // The Program list opens inside this, so it can neither move its top edge nor grow past the
    // panel. It must be a SIBLING of programHeader, never a child: that component narrows its
    // hitTest to the program window and the two buttons, and JUCE stops searching a component's
    // children once its own hitTest rejects the point - so a list parented there would be dead
    // everywhere except the cell it drops from.
    const int hostTop = ProgramHeader::menuHostTop();
    menuHost.setBounds(0, hostTop, getWidth(), getHeight() - hostTop);
    menuHost.setInterceptsMouseClicks(false, true);
    addAndMakeVisible(menuHost);
    menuHost.toFront(false);
    programHeader.setMenuParent(&menuHost);
}

GatecrasherEditorContent::~GatecrasherEditorContent()
{
    setLookAndFeel(nullptr);
}
