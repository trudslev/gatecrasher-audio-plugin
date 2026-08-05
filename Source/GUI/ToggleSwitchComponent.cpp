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

    // Option labels below, active one dark, inactive one grey (section 5/7). Their colour swaps
    // with the active side, so each repaint clears the previous frame back to the bare chassis
    // first. That erase is a genuine clear now: while the background was the fully dressed render it
    // also carried baked copies of these very words, which made erasing here a no-op (the restored
    // pixels were the text being redrawn) and forced an opaque flat fill instead - one that couldn't
    // reproduce the fascia's brush grain. Nothing is baked under them any more.
    //
    // Geometry (pair centred on the track, fixed gap, vertical centre a fixed drop below the track)
    // is all measured off the dressed reference render - see the switchOptionLabel* constants'
    // comment, which also covers why the font height is solved at runtime rather than hard-coded.
    static const float optionLabelHeight =
        labelFontHeightForTrackedWidth(Layout::switchOptionLabelRefText, Layout::switchOptionLabelTracking,
                                        Layout::switchOptionLabelRefWidth);
    const auto labelFontToUse = labelFont(optionLabelHeight);

    const juce::String textForZero = labelForZero.toUpperCase();
    const juce::String textForOne = labelForOne.toUpperCase();
    const float widthForZero = trackedTextWidth(textForZero, labelFontToUse, Layout::switchOptionLabelTracking);
    const float widthForOne = trackedTextWidth(textForOne, labelFontToUse, Layout::switchOptionLabelTracking);
    const float pairWidth = widthForZero + Layout::switchOptionLabelGapX + widthForOne;

    const float pairLeft = track.getCentreX() - pairWidth * 0.5f;
    const float rowTop = track.getBottom() + Layout::switchOptionLabelCentreBelowTrack
                          - Layout::switchLabelRowH * 0.5f;

    const juce::Rectangle<float> rectForZero(pairLeft, rowTop, widthForZero, Layout::switchLabelRowH);
    const juce::Rectangle<float> rectForOne(pairLeft + widthForZero + Layout::switchOptionLabelGapX, rowTop,
                                             widthForOne, Layout::switchLabelRowH);

    // Padded past the text pair so the clear also takes the previous frame's antialiased edges.
    eraseToBackground(g, rectForZero.getUnion(rectForOne).expanded(5.0f, 2.0f), getPosition());

    const bool zeroActive = thumbPosition01 < 0.5f;
    drawTrackedText(g, textForZero, labelFontToUse, Layout::switchOptionLabelTracking, rectForZero,
                     juce::Justification::centred,
                     zeroActive ? Colour::controlLabelText : Colour::inactiveLabel);
    drawTrackedText(g, textForOne, labelFontToUse, Layout::switchOptionLabelTracking, rectForOne,
                     juce::Justification::centred,
                     zeroActive ? Colour::inactiveLabel : Colour::controlLabelText);
}
