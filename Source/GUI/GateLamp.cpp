#include "GateLamp.h"
#include "GatecrasherTheme.h"
#include <cmath>

/*  §8.2's GATE OPEN lamp. Ø15 in a 31 px well at (272, 140).

    **Light stops at the lens edge.** The glow is drawn INSIDE the lens rather than around it, which
    makes that structural rather than something to remember: there is no halo on the fascia because
    nothing outside the lens is ever painted. And the closed lens stays a dark RED lens rather than
    going grey — §7 gives this panel one accent, and the unlit state is that accent gone dark, not a
    different material.  */

GateLamp::GateLamp(GatecrasherAudioProcessor& processor) : processorRef(processor)
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(30);
}

GateLamp::~GateLamp()
{
    stopTimer();
}

/*  Opening is INSTANTANEOUS - §5's most emphatic requirement - so the glow snaps to 1 on the poll
    that reads open and is never eased in. Closing gets a short cosmetic decay at this layer only;
    GateEnvelopeGenerator leaves it to the GUI deliberately, which decouples how the lamp looks as it
    goes dark from the audible release curve.  */
void GateLamp::timerCallback()
{
    constexpr float glowDecayPerFrame = 0.55f;   // ~40 ms to black at 30 Hz

    const bool open = processorRef.getGateEnvelope() > 0.02f;
    const float target = open ? 1.0f : displayedGlow * glowDecayPerFrame;
    const float glow = open ? 1.0f : (target < 0.01f ? 0.0f : target);

    if (std::abs (glow - displayedGlow) < 1.0e-4f)
        return;

    displayedGlow = glow;
    repaint();
}

void GateLamp::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const bool displayedOpen = displayedGlow > 0.5f;

    const juce::Rectangle<float> lens (Layout::lampCx - Layout::lampDiameter * 0.5f,
                                        Layout::lampCy - Layout::lampDiameter * 0.5f,
                                        Layout::lampDiameter, Layout::lampDiameter);

    /*  `radial-gradient(circle at 40% 32%, …)`. JUCE's radial gradient is circular, which is what
        the CSS asks for here, so this is one of the cases that maps directly - the conic and
        elliptical forms elsewhere in this suite are the ones that need sweeping by hand. */
    const float cx = lens.getX() + lens.getWidth() * 0.40f;
    const float cy = lens.getY() + lens.getHeight() * 0.32f;

    juce::ColourGradient lensFill (displayedOpen ? Colour::lampOpenCore : Colour::lampShutCore,
                                    cx, cy,
                                    displayedOpen ? Colour::lampOpenEdge : Colour::lampShutEdge,
                                    lens.getRight(), lens.getBottom(), true);
    if (displayedOpen)
    {
        lensFill.addColour (0.35, Colour::lampOpenMid);
        lensFill.addColour (0.70, Colour::lampOpenDeep);
    }
    else
    {
        lensFill.addColour (0.60, Colour::lampShutMid);
    }

    g.setGradientFill (lensFill);
    g.fillEllipse (lens);

    /*  The open state's `0 0 12px 3px` and `0 0 30px 8px` bloom, clipped to the lens. Outside it
        the two shadows would spill onto the fascia, which §8.2 forbids in the same sentence that
        specifies them - so the clip is what makes the spec's two halves consistent rather than a
        choice between them.  */
    if (displayedOpen)
    {
        juce::Graphics::ScopedSaveState glowState (g);
        g.reduceClipRegion (lens.getSmallestIntegerContainer());
        juce::ColourGradient bloom (Colour::gateAccent.withAlpha (0.55f * displayedGlow), Layout::lampCx, Layout::lampCy,
                                     Colour::gateAccent.withAlpha (0.0f),
                                     Layout::lampCx + Layout::lampDiameter, Layout::lampCy, true);
        g.setGradientFill (bloom);
        g.fillEllipse (lens.expanded (2.0f));
    }
    else
    {
        // `inset 0 -2px 4px rgba(0,0,0,.6)` - the closed lens sits deeper in its well.
        juce::Graphics::ScopedSaveState shadowState (g);
        g.reduceClipRegion (lens.getSmallestIntegerContainer());
        juce::ColourGradient recess (juce::Colours::black.withAlpha (0.0f),
                                      Layout::lampCx, lens.getBottom() - 6.0f,
                                      juce::Colours::black.withAlpha (0.6f),
                                      Layout::lampCx, lens.getBottom(), false);
        g.setGradientFill (recess);
        g.fillEllipse (lens);
    }
}
