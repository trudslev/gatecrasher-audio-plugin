#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Generic 2-position slide switch, reused for both KEY SOURCE (Internal/Sidechain) and SHAPE
// (Hard/Soft) per GATECRASHER-GUI-SPEC.md section 5's "reuses the KEY SOURCE switch verbatim".
// Visually a simple 2-position slide like TapeRot's ToggleSwitch; electrically it binds like
// TapeRot's NoiseCharacterSwitch - both keySource and shape are AudioParameterChoice (2 choices),
// not bool, so this subclasses juce::Slider (range 0..1, step 1) rather than juce::Button, for
// SliderAttachment compatibility.
//
// The static panel background already renders the caption and both option labels at their default
// state, so this component draws neither its own caption (never changes, drawing it live was pure
// redundant ghosting against the baked copy - re-blitting the background over a region that's
// already showing the exact same static text is a no-op, not an erase) nor a bare erase of the
// option labels - the two labels DO need to change colour live as the active side flips, but
// GatecrasherTheme::eraseToBackground alone isn't enough to make that safe (it's also a no-op
// whenever the switch is at its baked-default side, and even when it isn't, a live glyph never lands
// pixel-identical on the baked one under it), so paint() instead paints a flat fill in the fascia's
// own tone across the label row first, guaranteeing no baked pixel survives underneath regardless of
// state or alignment, and only then draws the current state on top. Getting this component's bounds
// to actually line up with where the baked assembly sits still matters for the caption and track -
// see GatecrasherTheme::shapeTrackY's comment for a case where they didn't.
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
