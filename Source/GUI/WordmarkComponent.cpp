#include "WordmarkComponent.h"
#include "GatecrasherTheme.h"

WordmarkComponent::WordmarkComponent()
{
    setInterceptsMouseClicks(false, false);
}

void WordmarkComponent::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    // TODO(design): section 8 specifies a pre-baked, transparent PNG wordmark - TudorVictors with
    // per-letter rotation (+/-1.9deg) and vertical drift (+/-1.1px), uneven per-letter opacity
    // (.80-.99), a soft speckle mask inside each glyph, a two-stage overspray halo (8px blur @30%
    // and 2.8px blur @34%), and five spatter flecks - shipped at @2x/@3x. That PNG doesn't exist
    // yet in design/assets/ (only the raw TudorVictors.ttf does), so this is an interim
    // placeholder: plain text in the embedded typeface, no spray/speckle/halo effects at all.
    // Replace this whole method with an Image blit at Layout::wordmarkX/Y once the real asset
    // ships - see gatecrasher/CLAUDE.md's Status section.
    juce::Font font = juce::Font(tudorVictorsTypeface()).withHeight(34.0f);
    g.setColour(juce::Colour(0xFF14171A));
    g.setFont(font);
    g.drawText("GATECRASHER",
               juce::Rectangle<float>(Layout::wordmarkX, Layout::wordmarkY, Layout::wordmarkW, Layout::wordmarkH),
               juce::Justification::centredLeft, false);
}
