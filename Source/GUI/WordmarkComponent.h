#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The "GATECRASHER" wordmark (GATECRASHER-GUI-SPEC.md section 8), blitted from a pre-baked
// transparent PNG - which is what section 8 specified from the start: "Do not render the sprayed
// lettering in code. Bake it."
//
// It briefly *was* drawn live from the embedded TudorVictors typeface, reproducing the mockup's
// per-letter rotation/drift/opacity, its two-stage overspray halo and its five spatter flecks. That
// had to stop for licensing rather than visual reasons: TudorVictors is Chequered Ink's, whose terms
// permit non-profit *use* but grant no right to redistribute the font file, and additionally allow
// embedding only in a form that "protects the Product from extraction" - which a BinaryData blob is
// not. Baking the one static string it ever rendered lets the .ttf leave both BinaryData and the
// repository while the artwork stays exactly as designed.
//
// The PNG was produced by tools/BakeWordmark.cpp (a one-shot, since deleted) rendering the previous
// live code offscreen at 3x, so it is pixel-faithful to what shipped before rather than a redraw.
// If the wordmark ever needs regenerating, recover that tool from this file's git history - it needs
// the .ttf placed locally, since the font is no longer tracked.
class WordmarkComponent final : public juce::Component
{
public:
    WordmarkComponent();

    void paint(juce::Graphics&) override;
};
