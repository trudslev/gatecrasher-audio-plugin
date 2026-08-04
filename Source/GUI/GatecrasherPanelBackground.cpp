#include "GatecrasherPanelBackground.h"
#include "GatecrasherTheme.h"

GatecrasherPanelBackground::GatecrasherPanelBackground()
{
    // Pure background - never intercepts mouse input, so every real control layered on top of it
    // (including ones that happen to sit within its bounds) still receives clicks normally.
    setInterceptsMouseClicks(false, false);
}

void GatecrasherPanelBackground::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(panelBackgroundImage(),
                juce::Rectangle<float>(0.0f, 0.0f, Layout::canvasWidth, Layout::canvasHeight),
                juce::RectanglePlacement::stretchToFit);
}
