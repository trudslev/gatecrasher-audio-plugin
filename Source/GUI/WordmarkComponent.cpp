#include "WordmarkComponent.h"
#include "GatecrasherTheme.h"

using namespace GatecrasherTheme;

WordmarkComponent::WordmarkComponent()
{
    setInterceptsMouseClicks(false, false);
}

void WordmarkComponent::paint(juce::Graphics& g)
{
    // A pre-baked, transparent PNG rather than live type - which is what GATECRASHER-GUI-SPEC.md
    // section 8 asked for in the first place ("Do not render the sprayed lettering in code. Bake
    // it."). See the header for the licensing reason it finally had to be done.
    //
    // The art is padded well beyond the nameplate block (see Layout::wordmarkArt*) because the
    // overspray halo and the spatter flecks both fall outside it.
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(wordmarkImage(),
                juce::Rectangle<float>(Layout::wordmarkArtX, Layout::wordmarkArtY,
                                        Layout::wordmarkArtW, Layout::wordmarkArtH),
                juce::RectanglePlacement::stretchToFit);
}
