#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Generic 2-position slide switch, reused for both KEY SOURCE (Internal/Sidechain) and SHAPE
// (Hard/Soft) per GATECRASHER-GUI-SPEC.md section 5's "reuses the KEY SOURCE switch verbatim".
// Visually a simple 2-position slide like TapeRot's ToggleSwitch; electrically it binds like
// TapeRot's NoiseCharacterSwitch - both keySource and shape are AudioParameterChoice (2 choices),
// not bool, so this subclasses juce::Slider (range 0..1, step 1) rather than juce::Button, for
// SliderAttachment compatibility. Draws its own caption above and the two option labels below
// (with live active/inactive colouring) rather than relying on any static painter, since Gatecrasher
// has no TapeRot-SectionPanel equivalent that could react to which position is currently active -
// see GatecrasherTheme::Layout's switch-assembly comment for how its bounds are derived.
class ToggleSwitchComponent final : public juce::Slider, private juce::Timer
{
public:
    ToggleSwitchComponent(juce::String captionText, juce::String label0, juce::String label1);
    ~ToggleSwitchComponent() override;

private:
    void paint(juce::Graphics&) override;
    void timerCallback() override;

    juce::String caption, labelForZero, labelForOne;

    // Eases the thumb between its two stops (rather than snapping) however the value changes - a
    // click, a drag, host automation, or a program load - same technique as TapeRot's
    // ToggleSwitch/NoiseCharacterSwitch.
    float thumbPosition01 = 0.0f;
    bool hasPaintedOnce = false;
};
