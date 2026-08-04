#pragma once

#include "GatecrasherTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

// A knob rendered from a 128-frame bitmap filmstrip (design/assets/knob_large_128px_128f.png /
// knob_small_128px_128f.png) rather than code-drawn - the deliberate GUI divergence from TapeRot's
// SmallKnob, see design/CLAUDE.md. Subclasses juce::Slider purely for its click/drag-to-value
// mapping and SliderAttachment compatibility (same pattern as TapeRotLookAndFeel::createKnobSlider),
// but paint() fully replaces the default look - LookAndFeel::drawRotarySlider is never invoked.
//
// The tick ring around the knob is drawn here in code (GATECRASHER-GUI-SPEC.md section 3: "draw in
// code, not part of the filmstrip - it does not rotate"), underneath the filmstrip frame so the
// frame's own baked cast-shadow bleed can naturally overlap the ring's inner edge.
class KnobFilmstripComponent final : public juce::Slider
{
public:
    // tickSpacingDegrees is the target spacing for the regular swept tick ring (ignored when
    // isAlgorithmSelector is true, which instead draws the 4 fixed diagonal ticks section 3
    // specifies for the algorithm selector).
    KnobFilmstripComponent(GatecrasherTheme::KnobFilmstripSize size, float diameterPx,
                            float tickSpacingDegrees, bool isAlgorithmSelector = false);

    void paint(juce::Graphics&) override;

private:
    GatecrasherTheme::KnobFilmstripSize filmstripSize;
    float diameter;
    float tickSpacingDegrees;
    bool algorithmSelectorTicks;
};
