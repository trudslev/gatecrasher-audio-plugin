#pragma once

#include "GatecrasherTheme.h"
#include "GatecrasherLookAndFeel.h"
#include "GatecrasherPanelBackground.h"
#include "KnobComponent.h"
#include "GateScope.h"
#include "GateLamp.h"
#include "InputMeter.h"
#include "ToggleSwitchComponent.h"
#include "ProgramHeader.h"
#include "../PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

// The assembler: owns every knob/switch/indicator plus their APVTS attachments and positions them
// from GatecrasherTheme::Layout, mirroring TapeRotEditorContent's role for TapeRot. Everything here
// draws in the fixed 960x434 reference canvas; PluginEditor applies the single uniform scale
// transform on resize.
class GatecrasherEditorContent final : public juce::Component
{
public:
    explicit GatecrasherEditorContent(GatecrasherAudioProcessor& processor);
    ~GatecrasherEditorContent() override;

private:
    GatecrasherAudioProcessor& processorRef;
    GatecrasherLookAndFeel lookAndFeel;

    /*  Draw order: the printed layer - fascia, rails, screws, dividers and every static string -
        then every live control above it. **The eight state-dependent labels are gone**: §6, §8.1
        and §8.3 make the legends printed once and never re-inked, so they are ink in
        `GatecrasherPanelBackground` and the shoe and the pointer carry the state. */
    GatecrasherPanelBackground panelBackground;

    // density/decay are deliberately automation-only APVTS parameters with no panel control
    // (GUI-SPEC.md section 9) - every other parameter gets a knob here, one per entry
    // in GatecrasherTheme::Layout::knobs (which itself omits those two).
    std::array<std::unique_ptr<KnobComponent>, GatecrasherTheme::Layout::knobs.size()> knobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
               GatecrasherTheme::Layout::knobs.size()> knobAttachments;

    ToggleSwitchComponent keySourceSwitch{"Key Source", "Internal", "Sidechain"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> keySourceAttachment;

    ToggleSwitchComponent shapeSwitch{"Shape", "Hard", "Soft"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeAttachment;

    GateScope gateScope;
    GateLamp gateLamp;
    InputMeter inputMeter;
    ProgramHeader programHeader;

    /** Paints nothing and claims no clicks of its own; it exists so the Program list has a parent
        area to be laid out in. Its bounds are what stop the list moving or overflowing the panel -
        see the constructor, and ../../CLAUDE.md's "The Program dropdown". */
    juce::Component menuHost;

    // Single shared popup for every knob's setTooltip() text below - scoped to this component
    // (rather than nullptr/whole-desktop) so it only ever considers Gatecrasher's own controls,
    // same pattern as TapeRotEditorContent's tooltipWindow.
    juce::TooltipWindow tooltipWindow{this};
};
