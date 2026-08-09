#include "GateLamp.h"
#include "GatecrasherTheme.h"

namespace
{
    // ~40ms cosmetic decay at 60fps (16.67ms/frame): exp(-16.67/40) per frame reaches ~1/e after
    // roughly one 40ms window, fully imperceptible within a handful of frames.
    constexpr float glowDecayPerFrame = 0.66f;
}

GateLamp::GateLamp(GatecrasherAudioProcessor& processor) : processorRef(processor)
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(60);
}

GateLamp::~GateLamp()
{
    stopTimer();
}

void GateLamp::timerCallback()
{
    if (processorRef.isGateOpen())
        displayedGlow = 1.0f; // instant - no fade-in, no smoothing (section 5)
    else
        displayedGlow *= glowDecayPerFrame;

    if (displayedGlow < 0.002f)
        displayedGlow = 0.0f;

    repaint();
}

void GateLamp::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const juce::Point<float> centre(Layout::lampCx, Layout::lampCy);
    const float r = Layout::lampDiameter * 0.5f;

    // Outer glow halo (two soft layers per spec's box-shadow values, approximated as a single
    // multi-stop radial gradient so it fades smoothly rather than reading as a hard-edged ring -
    // same technique as TapeRot's FailLamp).
    if (displayedGlow > 0.01f)
    {
        const float glowR = r + 30.0f;
        juce::ColourGradient glow(Colour::gateAccent.withAlpha(0.55f * displayedGlow), centre.x, centre.y,
                                   Colour::gateAccent.withAlpha(0.0f), centre.x + glowR, centre.y, true);
        glow.addColour(0.2, Colour::gateAccent.withAlpha(0.40f * displayedGlow));
        glow.addColour(0.45, Colour::gateAccent.withAlpha(0.22f * displayedGlow));
        glow.addColour(0.7, Colour::gateAccent.withAlpha(0.08f * displayedGlow));
        g.setGradientFill(glow);
        g.fillEllipse(centre.x - glowR, centre.y - glowR, glowR * 2.0f, glowR * 2.0f);
    }

    const float bulbX = centre.x - r, bulbY = centre.y - r, bulbD = r * 2.0f;

    // Closed base: flat unlit fill with a faint inset highlight (spec: "inset rgba(255,255,255,.14)
    // only" when closed) - drawn under the open-state overlay, at full strength only while closed.
    g.setColour(Colour::lampUnlit);
    g.fillEllipse(bulbX, bulbY, bulbD, bulbD);
    g.setColour(juce::Colours::white.withAlpha(0.14f * (1.0f - displayedGlow)));
    g.drawEllipse(bulbX + 1.0f, bulbY + 1.0f, bulbD - 2.0f, bulbD - 2.0f, 1.5f);

    // Open: radial gradient core -> mid @70% -> edge, its own alpha carrying the cosmetic decay
    // (the OPENING transition is unaffected by this fade since displayedGlow is already 1.0 on the
    // very poll the gate opens - see timerCallback).
    if (displayedGlow > 0.005f)
    {
        juce::ColourGradient bulbGradient(Colour::lampOpenCore.withAlpha(displayedGlow), centre.x, centre.y,
                                           Colour::lampOpenEdge.withAlpha(displayedGlow), centre.x + r, centre.y + r,
                                           true);
        bulbGradient.addColour(0.7, Colour::lampOpenMid.withAlpha(displayedGlow));
        g.setGradientFill(bulbGradient);
        g.fillEllipse(bulbX, bulbY, bulbD, bulbD);
    }
}
