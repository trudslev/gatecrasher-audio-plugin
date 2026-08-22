#pragma once

#include "GatecrasherTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

/*  A code-drawn knob: sweep arc, ticks, numerals, body and pointer. **This replaces
    KnobFilmstripComponent and both 160 px sheets**, which is §10's call 5 — already conformed in the
    artwork, since the ring, ticks, numerals and pointer were always drawn from rotation fractions;
    the sheets were the build's, not the design's.

    **The static layers are cached on DEVICE SCALE, not on component size.** Everything except the
    pointer is fixed for a given knob, so it is rendered once into an image and blitted; the pointer
    is drawn live over it. The cache key is `getPhysicalPixelScaleFactor()` because that is what
    actually changes the pixels — a component whose bounds never move can still be asked to paint at
    2x on one display and 1x on another, and `setBufferedToImage` would re-render on every repaint
    instead of on the one thing that matters.  */
class KnobComponent final : public juce::Slider
{
public:
    KnobComponent (const GatecrasherTheme::Layout::KnobSpec& spec,
                   const GatecrasherTheme::Layout::KnobScale& scale);

    void paint (juce::Graphics&) override;
    /** The hit area is the CAP, not the bounds: the numeral ring reaches well past the cap and a
        click on a numeral belongs to the panel, not to the knob. */
    void mouseDown (const juce::MouseEvent&) override;
    bool hitTest (int x, int y) override;

private:
    void renderStaticLayer (float deviceScale);

    const GatecrasherTheme::Layout::KnobSpec& knobSpec;
    const GatecrasherTheme::Layout::KnobScale& knobScale;

    juce::Image staticLayer;
    float cachedDeviceScale = 0.0f;
};
