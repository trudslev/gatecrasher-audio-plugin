#pragma once

#include "../PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// The program section (GATECRASHER-GUI-SPEC.md section 6): the three header-state bitmaps
// (factory/user/naming) as a structural base layer, with only the dynamic bits - the FACT/USER tag,
// the program name (or the in-progress typed name + blinking caret), and SAVE/DELETE press
// feedback - painted live on top. "Current program" isn't an APVTS parameter, so this polls
// getCurrentProgram()/getProgramName()/isFactoryProgram() on a timer to stay in sync with
// host-driven program changes, same pattern as TapeRot's PresetStrip.
//
// Sized to the full canvas (matching GateScope/GateLamp/InputMeter's convention for pure overlay
// components), with hitTest narrowed to just the header cluster's own bounds so it doesn't swallow
// clicks meant for knobs/switches elsewhere on the panel - see TapeRot's PresetStrip for the same
// pattern, minus the full-canvas OR-of-rects (everything interactive here already sits inside one
// bounding rect).
class ProgramHeader final : public juce::Component, private juce::Timer
{
public:
    explicit ProgramHeader(GatecrasherAudioProcessor& processor);
    ~ProgramHeader() override;

    void paint(juce::Graphics&) override;
    bool hitTest(int x, int y) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;

private:
    enum class HeaderButton { none, save, deleteOrCancel };

    void timerCallback() override;
    void refreshDisplayFromProcessor();
    void enterNamingMode();
    void commitStore();
    void cancelNaming();
    HeaderButton buttonAt(juce::Point<float>) const;
    bool isButtonEnabled(HeaderButton) const;

    GatecrasherAudioProcessor& processorRef;

    // Mirrors whatever program was loaded before SAVE was pressed - CANCEL reverts the display to
    // this without ever touching APVTS (the user's tweaked-but-unsaved knob values must survive a
    // Cancel, per section 6). Never written to while namingMode is true.
    int displayedProgramIndex = -1;
    juce::String displayedProgramName;
    bool displayedIsFactory = true;

    bool namingMode = false;
    juce::String typedName;

    HeaderButton pressedButton = HeaderButton::none;

    juce::Rectangle<float> saveButtonRect, deleteButtonRect, headerClusterRect;
    juce::Rectangle<float> tagCellRect, nameCellRect;
};
