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

    const juce::Rectangle<float> track(Layout::switchAssemblyPad, Layout::switchCaptionRowH,
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

    // Caption above (e.g. "SHAPE" / "KEY SOURCE"), section 5: 9px / .20em tracking.
    // TODO(design): Barlow Condensed 600 not yet in design/assets/ - default sans placeholder.
    drawTrackedText(g, caption.toUpperCase(), labelFont(9.0f), 1.8f,
                     juce::Rectangle<float>(0.0f, 0.0f, (float) getWidth(), Layout::switchCaptionRowH),
                     juce::Justification::centred, Colour::tertiaryGroupLabel);

    // Option labels below, active one dark, inactive one grey (section 5/7).
    const juce::Rectangle<float> labelRow(0.0f, Layout::switchCaptionRowH + Layout::switchTrackH,
                                           (float) getWidth(), Layout::switchLabelRowH);
    const auto leftHalf = labelRow.withWidth(labelRow.getWidth() * 0.5f);
    const auto rightHalf = labelRow.withX(labelRow.getCentreX()).withWidth(labelRow.getWidth() * 0.5f);

    const bool zeroActive = thumbPosition01 < 0.5f;
    const auto labelFontToUse = labelFont(9.0f);
    drawTrackedText(g, labelForZero.toUpperCase(), labelFontToUse, 0.4f, leftHalf, juce::Justification::centred,
                     zeroActive ? Colour::controlLabelText : Colour::inactiveLabel);
    drawTrackedText(g, labelForOne.toUpperCase(), labelFontToUse, 0.4f, rightHalf, juce::Justification::centred,
                     zeroActive ? Colour::inactiveLabel : Colour::controlLabelText);
}
