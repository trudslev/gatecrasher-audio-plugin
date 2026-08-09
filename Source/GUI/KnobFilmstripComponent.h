#pragma once

#include "GatecrasherTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

// A knob rendered from a 128-frame bitmap filmstrip (design/assets/knob_large_128px_128f.png /
// knob_small_128px_128f.png) rather than code-drawn - the deliberate GUI divergence from TapeRot's
// SmallKnob, see design/CLAUDE.md. Subclasses juce::Slider purely for its click/drag-to-value
// mapping and SliderAttachment compatibility, but paint() fully replaces the default look -
// LookAndFeel::drawRotarySlider is never invoked.
//
// This draws the filmstrip frame and NOTHING ELSE. The tick ring, the sweep arc, the printed
// numerals and the algorithm selector's detent ring are all baked into the panel plate (spec
// section 0.3), positioned at their LABELLED VALUES rather than at even angular subdivisions - four
// knobs are power-law skewed, so an evenly-spaced ring drawn in code would disagree with the
// numerals beside it. Rev 5 drew that ring at a fixed 15/21 degree spacing; it is gone.
class KnobFilmstripComponent final : public juce::Slider
{
public:
    KnobFilmstripComponent(GatecrasherTheme::KnobFilmstripSize size, float diameterPx);

    void paint(juce::Graphics&) override;
    bool hitTest(int x, int y) override;

private:
    GatecrasherTheme::KnobFilmstripSize filmstripSize;
    float diameter;
};
