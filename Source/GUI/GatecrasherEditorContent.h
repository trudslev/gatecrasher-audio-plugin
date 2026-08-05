#pragma once

#include "GatecrasherTheme.h"
#include "GatecrasherLookAndFeel.h"
#include "GatecrasherPanelBackground.h"
#include "KnobFilmstripComponent.h"
#include "GateScope.h"
#include "GateLamp.h"
#include "InputMeter.h"
#include "ToggleSwitchComponent.h"
#include "ProgramHeader.h"
#include "WordmarkComponent.h"
#include "PanelChrome.h"
#include "PanelReadouts.h"
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

    // Draw order matters: the bare chassis, then the engraved static layer on top of it, then every
    // live control above that. PanelReadouts sits with the live controls (its values change), but
    // below the knobs so a knob's cast-shadow bleed overlaps its own readout the way the artwork
    // shows rather than the text sitting on top of the knob.
    GatecrasherPanelBackground panelBackground;
    PanelChrome panelChrome;
    PanelReadouts panelReadouts;
    WordmarkComponent wordmark;

    // density/decay are deliberately automation-only APVTS parameters with no panel control
    // (GATECRASHER-GUI-SPEC.md section 9) - every other parameter gets a knob here, one per entry
    // in GatecrasherTheme::Layout::knobs (which itself omits those two).
    std::array<std::unique_ptr<KnobFilmstripComponent>, GatecrasherTheme::Layout::knobs.size()> knobs;
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

    // Single shared popup for every knob's setTooltip() text below - scoped to this component
    // (rather than nullptr/whole-desktop) so it only ever considers Gatecrasher's own controls,
    // same pattern as TapeRotEditorContent's tooltipWindow.
    juce::TooltipWindow tooltipWindow{this};
};
