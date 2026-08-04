#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The "GATECRASHER" wordmark (GATECRASHER-GUI-SPEC.md section 8). Owns only the wordmark's own
// bounding box, kept deliberately separate from ProgramHeader even though both the static panel
// background and the header-state bitmaps happen to already show a wordmark baked into their own
// artwork at roughly this position - see GatecrasherTheme::Layout::headerCropX's comment for why
// ProgramHeader's own blit stops short of that region. See the TODO(design) note in the .cpp for
// the interim placeholder this draws pending the real asset.
class WordmarkComponent final : public juce::Component
{
public:
    WordmarkComponent();

    void paint(juce::Graphics&) override;
};
