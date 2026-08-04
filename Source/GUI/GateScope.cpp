#include "GateScope.h"
#include "GatecrasherTheme.h"

namespace
{
    // Peak decays to ~4% after 30 frames (~0.5s at 60fps) - reads as a brisk exponential decay
    // rather than a sluggish VU-style fall, appropriate for a percussive trigger indicator.
    constexpr float triggerDisplayDecayPerFrame = 0.90f;
}

GateScope::GateScope(GatecrasherAudioProcessor& processor) : processorRef(processor)
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(60);
}

GateScope::~GateScope()
{
    stopTimer();
}

void GateScope::timerCallback()
{
    using namespace GatecrasherTheme;

    const float env = juce::jlimit(0.0f, 1.0f, processorRef.getGateEnvelope());
    const float rawTrigger = juce::jmax(0.0f, processorRef.getTriggerLevel());
    triggerDisplayDecayed = juce::jmax(rawTrigger, triggerDisplayDecayed * triggerDisplayDecayPerFrame);

    envelopeHistory[(size_t) writeIndex] = env;
    triggerDisplayHistory[(size_t) writeIndex] = juce::jlimit(0.0f, 1.0f, triggerDisplayDecayed);
    writeIndex = (writeIndex + 1) % historySize;

    gridScrollPhase = std::fmod(gridScrollPhase + Layout::scopePixelsPerFrame, Layout::scopeGridSpacing);

    repaint();
}

void GateScope::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const juce::Rectangle<float> outerRect(Layout::scopeX, Layout::scopeY, Layout::scopeW, Layout::scopeH);
    const auto innerRect = outerRect.reduced(Layout::scopeInnerInset);

    const float baselineY = outerRect.getY() + outerRect.getHeight() - Layout::scopeBaselineInset;
    const float ceilingY = outerRect.getY() + Layout::scopeCeilingInset;

    // Background gradient + border (section 5: 1px #0A0C0D border, 1px inner padding,
    // bg #06080A -> #0B0F11 vertical).
    juce::ColourGradient bgGradient(Colour::scopeBgTop, innerRect.getCentreX(), innerRect.getY(),
                                     Colour::scopeBgBottom, innerRect.getCentreX(), innerRect.getBottom(), false);
    g.setGradientFill(bgGradient);
    g.fillRect(innerRect);
    g.setColour(Colour::scopeBorder);
    g.drawRect(outerRect, 1.0f);

    g.saveState();
    g.reduceClipRegion(innerRect.getSmallestIntegerContainer());

    // Scrolling vertical grid, 44px pitch, moving in lockstep with the trace's own 2px/frame
    // scroll (section 5).
    g.setColour(Colour::scopeGrid);
    for (float x = innerRect.getRight() - gridScrollPhase; x >= innerRect.getX(); x -= Layout::scopeGridSpacing)
        g.drawVerticalLine((int) x, innerRect.getY(), innerRect.getBottom());

    // 5 static horizontal grid lines.
    for (int i = 0; i < Layout::scopeNumStaticHorizontals; ++i)
    {
        const float t = (float) i / (float) (Layout::scopeNumStaticHorizontals - 1);
        const float y = innerRect.getY() + t * innerRect.getHeight();
        g.drawHorizontalLine((int) y, innerRect.getX(), innerRect.getRight());
    }

    // Baseline, brighter than the general grid.
    g.setColour(Colour::scopeBaseline);
    g.drawHorizontalLine((int) baselineY, innerRect.getX(), innerRect.getRight());

    // Build this frame's visible column list (oldest -> newest, left -> right) from the local
    // history ring buffer.
    const int visibleColumns = juce::jmin(historySize,
        (int) std::ceil(innerRect.getWidth() / Layout::scopePixelsPerFrame) + 1);

    juce::Path envelopePath, fillPath;
    bool firstColumn = true;
    float lastX = innerRect.getRight();

    for (int col = 0; col < visibleColumns; ++col)
    {
        const int age = visibleColumns - 1 - col; // 0 = newest (rightmost)
        const int idx = ((writeIndex - 1 - age) % historySize + historySize) % historySize;
        const float x = innerRect.getRight() - (float) age * Layout::scopePixelsPerFrame;
        if (x < innerRect.getX() - Layout::scopePixelsPerFrame)
            continue;

        // Grey input-waveform underlay - 1px vertical strokes, grey, never red (section 5).
        const float triggerY = baselineY - triggerDisplayHistory[(size_t) idx] * (baselineY - ceilingY);
        g.setColour(Colour::scopeInputWaveform);
        g.drawVerticalLine((int) x, triggerY, baselineY);

        const float envY = baselineY - envelopeHistory[(size_t) idx] * (baselineY - ceilingY);
        if (firstColumn)
        {
            envelopePath.startNewSubPath(x, envY);
            fillPath.startNewSubPath(x, baselineY);
            fillPath.lineTo(x, envY);
            firstColumn = false;
        }
        else
        {
            envelopePath.lineTo(x, envY);
            fillPath.lineTo(x, envY);
        }
        lastX = x;
    }

    if (!firstColumn)
    {
        fillPath.lineTo(lastX, baselineY);
        fillPath.closeSubPath();

        // Fill beneath the trace.
        juce::ColourGradient fillGradient(Colour::scopeFillTop, innerRect.getCentreX(), ceilingY,
                                           Colour::scopeFillBottom, innerRect.getCentreX(), baselineY, false);
        g.setGradientFill(fillGradient);
        g.fillPath(fillPath);

        // Glow: a blurred copy of the trace's own stroked outline (not the raw centreline, which
        // has zero fillable area) in the same red (section 5).
        juce::Path strokeOutline;
        juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt)
            .createStrokedPath(strokeOutline, envelopePath);
        juce::DropShadow glow(Colour::gateAccent.withAlpha(0.55f), 10, {0, 0});
        glow.drawForPath(g, strokeOutline);

        // The trace itself - hard mitre joins, no smoothing (section 5 / "10. What matters most").
        g.setColour(Colour::gateAccent);
        g.strokePath(envelopePath,
                     juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    }

    g.restoreState();

    // Annotations, Share Tech Mono placeholder (see GatecrasherTheme::monoFont's TODO).
    g.setColour(Colour::scopeAnnotation);
    g.setFont(monoFont(9.0f));
    g.drawText("GATE ENV", juce::Rectangle<float>(innerRect.getX() + 4.0f, innerRect.getY() + 2.0f, 80.0f, 12.0f),
               juce::Justification::centredLeft, false);
    g.drawText("0 dB", juce::Rectangle<float>(innerRect.getRight() - 50.0f, innerRect.getY() + 2.0f, 46.0f, 12.0f),
               juce::Justification::centredRight, false);
    g.drawText(juce::String(juce::CharPointer_UTF8("-\xe2\x88\x9e")),
               juce::Rectangle<float>(innerRect.getRight() - 50.0f, innerRect.getBottom() - 14.0f, 46.0f, 12.0f),
               juce::Justification::centredRight, false);
}
