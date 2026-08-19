#include "InputMeter.h"
#include "GatecrasherTheme.h"
#include "../Parameters.h"

InputMeter::InputMeter(GatecrasherAudioProcessor& processor) : processorRef(processor)
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(30);
}

InputMeter::~InputMeter()
{
    stopTimer();
}

void InputMeter::timerCallback()
{
    repaint();
}

/*  §4's IN ladder: 13 segments in 22 x 90 at (196, 214), 1 px gaps, radius 1.

    **Lit against unlit measures 16.59:1**, which §4 states as the property being bought - the ladder
    reads as a LEVEL rather than as a colour change. That is a graphic ratio, so no text floor
    applies and the two pairs carry no contrast annotation.

    The segment height is derived from the count and the gaps rather than stated, so a change to
    either cannot leave a stale third figure behind.  */
void InputMeter::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const float level = juce::jlimit (0.0f, 1.0f, processorRef.getInputMeterLevel());
    const int lit = juce::roundToInt (level * (float) Layout::meterSegmentCount);

    for (int i = 0; i < Layout::meterSegmentCount; ++i)
    {
        // Index 0 is the BOTTOM segment: the ladder fills upward, so the topmost box is the last to
        // light rather than the first.
        const int fromTop = Layout::meterSegmentCount - 1 - i;
        const juce::Rectangle<float> seg (Layout::meterX,
                                           Layout::meterY + (float) fromTop * Layout::meterSegmentPitch,
                                           Layout::meterW, Layout::meterSegmentH);

        const bool on = i < lit;
        juce::ColourGradient face (on ? Colour::ladderLitTop : Colour::ladderDarkTop,
                                    seg.getCentreX(), seg.getY(),
                                    on ? Colour::ladderLitBottom : Colour::ladderDarkBottom,
                                    seg.getCentreX(), seg.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (seg, Layout::meterSegmentRadius);

        // `inset 0 0 0 1px` - the ring is inside the box, so it is stroked on the inset rect.
        g.setColour (on ? Colour::ladderLitRing : Colour::ladderDarkRing);
        g.drawRoundedRectangle (seg.reduced (0.5f), Layout::meterSegmentRadius, 1.0f);
    }
}
