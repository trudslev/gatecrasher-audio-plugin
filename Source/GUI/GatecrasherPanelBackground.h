#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/*  **The printed layer, drawn — this used to be one bitmap blit and now draws every static mark on
    the panel.** §0: no plate, no filmstrips, no bitmap of any panel element. The fascia is a 2 px
    procedural repeat with nothing that wants baking, so call 6's per-casting permission applies.

    What it owns: the fascia, the two rack rails, the four screws, §1's three column dividers, and
    every printed string on the fascia — eight section headings, four corner labels, four shoe
    legends, the ladder caption, the GATE OPEN legend and the scope's header data.

    What it does not: anything that changes. The knobs, pointers, lamp, LCD, meter fill, scope
    contents and both shoes composite on top.

    Regenerate the object list rather than maintaining one here:

        python3 ../tools/enumerate_prototype.py "design/Gatecrasher GR-85 Panel.dc.html" \
            --canvas 1340x700
*/
class GatecrasherPanelBackground final : public juce::Component
{
public:
    GatecrasherPanelBackground();

    void paint(juce::Graphics&) override;

private:
    void paintPrintedLabels(juce::Graphics&);
};
