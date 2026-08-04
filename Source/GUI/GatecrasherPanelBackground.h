#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The flat static background: design/assets/gatecrasher-panel@2x.png (gate-closed), drawn once
// across the full 960x434 reference canvas. Fascia gradient/grain, rack ears, screws, dividers, and
// every static label are all baked into this single bitmap - see design/CLAUDE.md's "GUI approach"
// and gatecrasher/CLAUDE.md's GUI section. Every other GUI component in this plugin is layered on
// top of this one, either painting over a specific region (ProgramHeader, the switches) or adding
// something the bitmap can't show at all (the knobs, the scope, the lamp).
class GatecrasherPanelBackground final : public juce::Component
{
public:
    GatecrasherPanelBackground();

    void paint(juce::Graphics&) override;
};
