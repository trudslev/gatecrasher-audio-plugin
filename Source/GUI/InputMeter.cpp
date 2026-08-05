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

    // The static panel background bakes this meter in showing the reference mockup's own signal
    // level - several lit segments near the bottom. The unlit ledger drawn below only covers the 4px
    // segments themselves, not the 2px gaps between them on the 6px pitch, so those baked lit
    // segments kept showing through the gaps as stray bright lines that never tracked the real input
    // level. Filling the whole window dark first means only live segments are ever visible. Flat
    // (rather than GatecrasherTheme::eraseToBackground) for the same reason the switch labels are:
    // erasing here would just restore the baked lit segments, which is the thing being fixed.
    g.setColour(Colour::ledWindowBg);
    g.fillRect(rect);

    // Segments are 4px tall on a 6px pitch, anchored to the bottom edge, stacking upward.
    for (int i = 0; i < segmentCount; ++i)
    {
        const float segBottom = rect.getBottom() - (float) i * Layout::meterSegmentPitch;
        const juce::Rectangle<float> seg(rect.getX(), segBottom - Layout::meterSegmentH,
                                          rect.getWidth(), Layout::meterSegmentH);
        const bool lit = i < litCount;

        if (lit)
        {
            g.setColour(Colour::meterBloom);
            g.fillRect(seg.expanded(1.5f, 1.5f));
        }

        g.setColour(lit ? Colour::meterLitSegment : Colour::meterUnlitSegment);
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
