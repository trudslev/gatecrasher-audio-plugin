#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Everything engraved on the fascia that never changes: section headings, group labels, each knob's
// label, the switch captions, the header's nameplate subtitle and PROGRAM / IN / OUT captions, the
// recessed LED window frames, the input meter's frame, the scope's annotations, and the version
// stamp.
//
// This exists because design/assets/gatecrasher-panel-bare@2x.png is a BARE chassis - fascia,
// rails, header band and dividers only. An earlier version of this GUI instead used the fully
// dressed render (gatecrasher-panel@2x.png) as its background, which meant every live element sat
// on top of a baked copy of itself: the knobs over a baked knob (whose frozen pointer showed through
// as a second needle), the switch labels over baked labels (ghosting whenever the live glyph didn't
// land pixel-identically on the baked one), the meter over baked lit segments that showed through
// the gaps between the live ones, and the IN/OUT windows over a frozen reading that never moved.
// Each of those needed its own erase-or-cover workaround, several of which could only paper over the
// symptom - re-blitting the background across a region whose baked content is exactly what you are
// about to redraw is a no-op, so those had to use opaque flat fills that then clashed with the
// fascia grain around them. Drawing the static layer here instead removes the whole class of
// problem: nothing is underneath any live element except plain chassis.
//
// Sizes are quoted in the spec and the reference mockup as CSS px and converted through
// GatecrasherTheme::labelFontHeightForCssPx (see its comment - a juce::Font height is not the same
// number). Positions were measured off the dressed render, which remains in design/assets/ as the
// pixel-matching acceptance target even though it is no longer shipped in BinaryData.
//
// Purely static: painted once, no timer, and it never intercepts mouse input. Anything whose
// appearance depends on plugin state lives elsewhere - PanelReadouts for the numeric values and the
// algorithm selector's lit corner label, and the individual control components for the rest.
class PanelChrome final : public juce::Component
{
public:
    PanelChrome();

    void paint(juce::Graphics&) override;

private:
    void paintHeader(juce::Graphics&);
    void paintSectionHeadings(juce::Graphics&);
    void paintKnobLabels(juce::Graphics&);
    void paintSwitchCaptions(juce::Graphics&);
    void paintInputMeterFrame(juce::Graphics&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelChrome)
};
