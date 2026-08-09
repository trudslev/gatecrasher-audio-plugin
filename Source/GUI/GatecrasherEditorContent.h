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
#include "StateLabels.h"
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

    // Draw order follows spec section 0.5: the printed plate, then the eight state-dependent
    // labels, then every live control above them.
    GatecrasherPanelBackground panelBackground;
    StateLabels stateLabels;

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
