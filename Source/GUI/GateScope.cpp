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

    // Section 5.1's three nested rectangles. Keeping them distinct is the whole point: the trace,
    // fill, underlay and grid are clipped to the PLOT REGION, never to the dark rect - clipping to
    // the dark rect lets the trace run under the scale gutter and collide with the annotations,
    // which is the one mistake that section exists to prevent.
    const juce::Rectangle<float> darkRect(Layout::scopeDarkX, Layout::scopeDarkY,
                                           Layout::scopeDarkW, Layout::scopeDarkH);
    const juce::Rectangle<float> plotRect(darkRect.getX() + Layout::scopePlotLocalX,
                                           darkRect.getY() + Layout::scopePlotLocalY,
                                           Layout::scopePlotW, Layout::scopePlotH);

    const float baselineY = plotRect.getBottom();
    const float ceilingY = plotRect.getY();

    // The dark rect's own backing, section 5.1: #0B0F11 -> #050708 vertical. The well's recess and
    // its 1px border are baked into the plate; nothing draws them here.
    juce::ColourGradient bgGradient(Colour::scopeBgTop, darkRect.getCentreX(), darkRect.getY(),
                                     Colour::scopeBgBottom, darkRect.getCentreX(), darkRect.getBottom(), false);
    g.setGradientFill(bgGradient);
    g.fillRect(darkRect);

    // The two reserved strips, flat and slightly darker so they read as chrome rather than as part
    // of the plot, with their 1px separating rules.
    const juce::Rectangle<float> titleStrip(darkRect.getX(), darkRect.getY(),
                                             darkRect.getWidth(), Layout::scopeTitleStripH);
    const juce::Rectangle<float> gutterStrip(darkRect.getX() + Layout::scopeGutterLocalX,
                                              darkRect.getY() + Layout::scopeTitleStripH,
                                              Layout::scopeGutterW,
                                              darkRect.getHeight() - Layout::scopeTitleStripH);
    g.setColour(Colour::scopeStrip);
    g.fillRect(titleStrip);
    g.fillRect(gutterStrip);

    g.setColour(Colour::scopeStripRule);
    g.drawHorizontalLine((int) titleStrip.getBottom(), darkRect.getX(), darkRect.getRight());
    g.drawVerticalLine((int) gutterStrip.getX(), gutterStrip.getY(), gutterStrip.getBottom());

    g.saveState();
    g.reduceClipRegion(plotRect.getSmallestIntegerContainer());

    // Scrolling vertical grid, 44px pitch, moving in lockstep with the trace's own 2px/frame
    // scroll (section 5).
    g.setColour(Colour::scopeGrid);
    for (float x = plotRect.getRight() - gridScrollPhase; x >= plotRect.getX(); x -= Layout::scopeGridSpacing)
        g.drawVerticalLine((int) x, plotRect.getY(), plotRect.getBottom());

    // 5 static horizontal grid lines.
    for (int i = 0; i < Layout::scopeNumStaticHorizontals; ++i)
    {
        const float t = (float) i / (float) (Layout::scopeNumStaticHorizontals - 1);
        const float y = plotRect.getY() + t * plotRect.getHeight();
        g.drawHorizontalLine((int) y, plotRect.getX(), plotRect.getRight());
    }

    // Baseline, brighter than the general grid.
    g.setColour(Colour::scopeBaseline);
    g.drawHorizontalLine((int) baselineY, plotRect.getX(), plotRect.getRight());

    // Build this frame's visible column list (oldest -> newest, left -> right) from the local
    // history ring buffer.
    const int visibleColumns = juce::jmin(historySize,
        (int) std::ceil(plotRect.getWidth() / Layout::scopePixelsPerFrame) + 1);

    juce::Path envelopePath, fillPath;
    bool firstColumn = true;
    float lastX = plotRect.getRight();

    for (int col = 0; col < visibleColumns; ++col)
    {
        const int age = visibleColumns - 1 - col; // 0 = newest (rightmost)
        const int idx = ((writeIndex - 1 - age) % historySize + historySize) % historySize;
        const float x = plotRect.getRight() - (float) age * Layout::scopePixelsPerFrame;
        if (x < plotRect.getX() - Layout::scopePixelsPerFrame)
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
        juce::ColourGradient fillGradient(Colour::scopeFillTop, plotRect.getCentreX(), ceilingY,
                                           Colour::scopeFillBottom, plotRect.getCentreX(), baselineY, false);
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

    // Annotations sit in the two reserved strips, OUTSIDE the plot clip - that separation is why
    // the strips exist. Section 0.2 marks these R: they are drawn with the scope, not baked.
    g.setColour(Colour::scopeAnnotation);
    g.setFont(monoFont(monoFontHeightForCssPx(9.0f)));
    g.drawText("GATE ENV", titleStrip.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);
    g.drawText("0 dB", gutterStrip.withHeight(14.0f).reduced(3.0f, 0.0f),
               juce::Justification::centredRight, false);
    g.drawText(juce::String(juce::CharPointer_UTF8("-\xe2\x88\x9e")),
               gutterStrip.withTop(gutterStrip.getBottom() - 14.0f).reduced(3.0f, 0.0f),
               juce::Justification::centredRight, false);
}
