#include "ToggleSwitchComponent.h"
#include "GatecrasherTheme.h"

ToggleSwitchComponent::ToggleSwitchComponent(juce::String captionText, juce::String label0, juce::String label1)
    : caption(std::move(captionText)), labelForZero(std::move(label0)), labelForOne(std::move(label1))
{
    setSliderStyle(juce::Slider::LinearHorizontal);
    setRange(0.0, 1.0, 1.0);
    setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    startTimerHz(60);
}

ToggleSwitchComponent::~ToggleSwitchComponent()
{
    stopTimer();
}

void ToggleSwitchComponent::timerCallback()
{
    const float target = (float) std::round(getValue());
    if (std::abs(target - thumbPosition01) > 0.001f)
    {
        thumbPosition01 += (target - thumbPosition01) * 0.35f;
        repaint();
    }
    else if (thumbPosition01 != target)
    {
        thumbPosition01 = target;
        repaint();
    }
}

/*  §6's two-state shoe, the shared §4B part: 128 x 32 in two 64 halves, radius 3, and the engaged
    half is the light one.

    **This component draws the frame and the two halves. Nothing else.** The legends are printed
    once, under their own half, in the panel's printed layer — they are not re-inked, not re-weighted
    and not moved, because the shoe carries the state and does not relabel the control. That is what
    makes this a port of the shared part rather than a resize of what was here: the old construction
    drew both labels itself, every frame, at two weights, and had to paint its own tone across the
    label row first so no baked pixel survived underneath.

    The thumb easing stays. It reads better than a snap however the value changes — a click, a drag,
    host automation or a Program load — and §8.3's four cells are about which half is light rather
    than about how it gets there.  */
void ToggleSwitchComponent::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    if (! hasPaintedOnce)
    {
        // Snap to the real initial value on the very first paint: only changes made while the
        // switch is already on screen should visibly ease between stops.
        thumbPosition01 = (float) std::round (getValue());
        hasPaintedOnce = true;
    }

    const juce::Rectangle<float> shoe (0.0f, 0.0f, Layout::shoeW, Layout::shoeH);
    const float halfW = Layout::shoeW * 0.5f;

    // `0 1px 2px rgba(0,0,0,.3)` under the whole part.
    g.setColour (Colour::shoeDrop);
    g.fillRoundedRectangle (shoe.translated (0.0f, 1.0f), Layout::shoeRadius);

    juce::Graphics::ScopedSaveState clipState (g);
    {
        juce::Path rounded;
        rounded.addRoundedRectangle (shoe, Layout::shoeRadius);
        g.reduceClipRegion (rounded);
    }

    for (int half = 0; half < 2; ++half)
    {
        const juce::Rectangle<float> face (shoe.getX() + (float) half * halfW, shoe.getY(),
                                            halfW, shoe.getHeight());

        // Engagement is continuous while the thumb eases, so the two halves cross-fade rather than
        // swapping on a frame - which is what makes the ease visible at all on a part with no
        // travelling thumb to watch.
        const float engaged = half == 0 ? 1.0f - thumbPosition01 : thumbPosition01;

        juce::ColourGradient idle (Colour::shoeIdleTop, face.getCentreX(), face.getY(),
                                    Colour::shoeIdleBottom, face.getCentreX(), face.getBottom(), false);
        g.setGradientFill (idle);
        g.fillRect (face);

        if (engaged > 0.0f)
        {
            juce::ColourGradient lit (Colour::shoeEngagedTop.withAlpha (engaged),
                                       face.getCentreX(), face.getY(),
                                       Colour::shoeEngagedBottom.withAlpha (engaged),
                                       face.getCentreX(), face.getBottom(), false);
            g.setGradientFill (lit);
            g.fillRect (face);

            // `inset 0 1px 0 rgba(255,255,255,.9)` - inside the half, so it reads as a lit face
            // rather than as a border on it.
            g.setColour (Colour::shoeEngagedLip.withAlpha (0.9f * engaged));
            g.fillRect (face.getX(), face.getY(), face.getWidth(), 1.0f);
        }
    }

    // `inset 0 0 0 1px #7c8286`, stroked last so nothing is drawn over it.
    g.setColour (Colour::shoeFrame);
    g.drawRoundedRectangle (shoe.reduced (0.5f), Layout::shoeRadius, 1.0f);
}
