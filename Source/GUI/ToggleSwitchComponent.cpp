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

void ToggleSwitchComponent::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    if (!hasPaintedOnce)
    {
        // Snap to the real initial value on the very first paint - only changes made after the
        // switch is already on screen should visibly ease between stops.
        thumbPosition01 = (float) std::round(getValue());
        hasPaintedOnce = true;
    }

    // The caption ("KEY SOURCE" / "SHAPE") is static, so PanelChrome engraves it along with every
    // other fixed label rather than this component redrawing it on every thumb movement.
    const float originX = Layout::switchLabelOverflowPad;
    const float originY = Layout::switchVerticalSafetyPad;

    const juce::Rectangle<float> track(originX + Layout::switchAssemblyPad, originY + Layout::switchCaptionRowH,
                                        Layout::switchTrackW, Layout::switchTrackH);

    g.setColour(Colour::switchTrackBg);
    g.fillRect(track);
    g.setColour(Colour::switchTrackBorder);
    g.drawRect(track, 1.0f);

    // Shoe: "left: 1px for the first position and left: 29px for the second" (section 5), eased
    // between the two stops via thumbPosition01.
    const float shoeH = Layout::switchTrackH - 4.0f;
    const float shoeLeft = 1.0f + thumbPosition01 * 28.0f;
    const juce::Rectangle<float> shoe(track.getX() + shoeLeft, track.getY() + 2.0f, Layout::switchShoeW, shoeH);

    juce::ColourGradient shoeGradient(Colour::switchShoeTop, shoe.getX(), shoe.getY(),
                                       Colour::switchShoeBottom, shoe.getX(), shoe.getBottom(), false);
    g.setGradientFill(shoeGradient);
    g.fillRect(shoe);

    // The option labels (INTERNAL/SIDECHAIN, HARD/SOFT) are NOT drawn here. They are two of the
    // four shoe legends of §6 and belong to GatecrasherPanelBackground's printed layer, which draws
    // eight together from one table so the switch pair and the algorithm corners cannot drift apart.
    // This component owns the track and the shoe only.
}
