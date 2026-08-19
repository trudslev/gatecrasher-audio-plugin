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
    : processorRef(p), gateScope(p), gateLamp(p), inputMeter(p), programHeader(p)
{
    setSize((int) Layout::canvasWidth, (int) Layout::canvasHeight);
    setLookAndFeel(&lookAndFeel);

    panelBackground.setBounds(getLocalBounds());
    addAndMakeVisible(panelBackground);

    // **There is no plate.** GatecrasherPanelBackground is the printed layer now - fascia, rails,
    // screws, dividers and every static string - and the nameplate is drawn by ProgramHeader. What
    // layers above is only what changes.

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
        // The same guard disarms the processor's stale-replay gate, because this is the only place
        // that knows a change came from a PERSON. It deliberately does not fire for automation: a
        // host may write automation on session load before replaying its remembered program index,
        // and disarming there would let that replay land on the restored state. One call rather than
        // two adjacent ones, so the disarm cannot be written without the hand-off - see
        // nf/UserEditGate.h.
        nf::connectUserEdit(*rawKnob, processorRef.userEdits,
                            [this, paramID] { programHeader.showParameter(paramID); });
        knob->onDragEnd = [this] { programHeader.releaseParameter(); };

        addAndMakeVisible(*knob);
        knobs[i] = std::move(knob);
    }

    /*  **The shoe is exactly its own 128 x 32 now.** The old bounds were the track's footprint
        widened by an overflow pad each side and a safety pad top and bottom, because the component
        drew a caption above and two labels below and had to erase a label row wider than itself.
        It draws neither: the legends are printed once in the panel's printed layer.

        A component sized to what it draws is also one whose hit area is what it looks like, which
        the padded version was not - it claimed 45 px of bare fascia on each side. */
    auto placeSwitch = [](ToggleSwitchComponent& sw, float x, float y)
    {
        sw.setBounds ((int) std::round (x), (int) std::round (y),
                       (int) std::round (Layout::shoeW), (int) std::round (Layout::shoeH));
    };

    /*  **Both switches report to the LCD, like every knob.** BRAND.md's rule is that every control
        changing a parameter announces itself there, switches included - a rule about which controls
        are "self-explanatory" is harder to apply consistently than no rule at all, and a switch is
        often the LEAST obvious thing on a panel: turning a knob shows you its own printed scale,
        while flipping a switch shows you nothing.

        Guarded on isMouseButtonDown for exactly the reason the knobs are, and disarming the
        stale-replay gate for exactly the same one: a SliderAttachment fires on Program recall and on
        every automation step, and this is the only place that knows a change came from a person.

        A switch settles the moment it is thrown, so there is no drag to end - it announces and
        releases inside the one callback rather than needing an onDragEnd. */
    const auto reportSwitch = [this] (ToggleSwitchComponent& sw, juce::String paramID)
    {
        nf::connectUserEdit(sw, processorRef.userEdits, [this, paramID]
        {
            programHeader.showParameter(paramID);
            programHeader.releaseParameter();
        });
    };

    placeSwitch(keySourceSwitch, Layout::shoeKeySourceX, Layout::shoeKeySourceY);
    reportSwitch(keySourceSwitch, ParamIDs::keySource);
    keySourceSwitch.setTooltip("Trigger detector source: Internal (main input) or Sidechain.");
    keySourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, ParamIDs::keySource, keySourceSwitch);
    addAndMakeVisible(keySourceSwitch);

    placeSwitch(shapeSwitch, Layout::shoeShapeX, Layout::shoeShapeY);
    reportSwitch(shapeSwitch, ParamIDs::shape);
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
