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

void InputMeter::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const juce::Rectangle<float> rect(Layout::meterX, Layout::meterY, Layout::meterW, Layout::meterH);

    const float levelLinear = juce::jmax(0.0f, processorRef.getInputMeterLevel());
    const float levelDb = juce::Decibels::gainToDecibels(levelLinear, Layout::meterFloorDb);
    const float levelNorm = juce::jlimit(0.0f, 1.0f,
        (levelDb - Layout::meterFloorDb) / (Layout::meterCeilingDb - Layout::meterFloorDb));

    const int segmentCount = (int) (Layout::meterH / Layout::meterSegmentPitch);
    const int litCount = (int) std::round(levelNorm * (float) segmentCount);

    // Section 8: "Well and unlit ledger baked; lit segments runtime." So this draws ONLY the lit
    // segments, and erasing to the plate is a genuine clear of the previous frame - the plate has an
    // unlit ledger and no lit segments at all. Under Rev 5's dressed background this same erase
    // restored a baked signal level and the code filled the window flat dark instead to defeat it;
    // that fill would now paint out the printed ledger, so it is gone with the background it was
    // fighting. Expanded by the bloom radius so the previous frame's glow is cleared too.
    eraseToBackground(g, rect.expanded(2.0f), getPosition());

    // Segments are 4px tall on a 6px pitch, anchored to the bottom edge, stacking upward.
    for (int i = 0; i < litCount && i < segmentCount; ++i)
    {
        const float segBottom = rect.getBottom() - (float) i * Layout::meterSegmentPitch;
        const juce::Rectangle<float> seg(rect.getX(), segBottom - Layout::meterSegmentH,
                                          rect.getWidth(), Layout::meterSegmentH);

        g.setColour(Colour::meterBloom);
        g.fillRect(seg.expanded(1.5f, 1.5f));
        g.setColour(Colour::meterLitSegment);
        g.fillRect(seg);
    }

    // Threshold marker - tracks the live Threshold parameter so it always shows where the gate
    // will actually trigger, not just a fixed reference line.
    if (auto* thresholdRaw = processorRef.apvts.getRawParameterValue(ParamIDs::threshold))
    {
        const float thresholdDb = thresholdRaw->load();
        const float thresholdNorm = juce::jlimit(0.0f, 1.0f,
            (thresholdDb - Layout::meterFloorDb) / (Layout::meterCeilingDb - Layout::meterFloorDb));
        const float markerY = rect.getBottom() - thresholdNorm * rect.getHeight();
        g.setColour(Colour::meterThresholdMarker);
        g.drawHorizontalLine((int) markerY, rect.getX(), rect.getRight());
    }
}
