#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Deliberately thin compared to TapeRotLookAndFeel: Gatecrasher's controls are asset-based and
// paint themselves entirely in their own paint() overrides (KnobFilmstripComponent,
// ToggleSwitchComponent, etc.) rather than going through LookAndFeel::drawRotarySlider/drawButton -
// see design/GUI-SPEC.md's "GUI approach". This class's job is just the handful of shared JUCE
// chrome that isn't owned by any one component: the fallback window background (visible only at
// the fixed-aspect-ratio letterboxing edges, if any) and the knob drag-value popup / shared
// TooltipWindow colours, kept consistent with the fascia palette.
class GatecrasherLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    GatecrasherLookAndFeel();
};
