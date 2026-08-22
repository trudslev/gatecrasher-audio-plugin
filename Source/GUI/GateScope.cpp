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

    gridScrollPhase = std::fmod(gridScrollPhase + Layout::scopePixelsPerFrame, Layout::scopeGridSpacingX);

    repaint();
}

void GateScope::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    /*  **§5's well, and the split that is the whole point of the section.** The well is 400 x 184;
        the PLOT is its left 358 and the LEGEND GUTTER the remaining 42, divided by a 1 px rule at
        x 358. The trace, its fill and the input underlay clip to the plot and never to the well —
        clipping to the well lets a full-height envelope run under `0 dB` and `−∞` and collide with
        them, which is exactly what the gutter exists to prevent. */
    const juce::Rectangle<float> well (Layout::scopeWellX, Layout::scopeWellY,
                                        Layout::scopeWellW, Layout::scopeWellH);
    const juce::Rectangle<float> plot (well.getX() + Layout::scopePlotLocalX,
                                        well.getY() + Layout::scopePlotLocalY,
                                        Layout::scopePlotW, Layout::scopePlotH);
    const juce::Rectangle<float> gutter (well.getX() + Layout::scopeGutterLocalX, well.getY(),
                                          Layout::scopeGutterW, well.getHeight());

    const float baselineY = well.getY() + Layout::scopeTraceBaselineLocalY;
    const float ceilingY  = plot.getY();

    // Flat glass. It was a vertical gradient with its recess and border baked into the plate; the
    // plate is gone, so the well draws its own frame.
    g.setColour (Colour::scopeGlass);
    g.fillRoundedRectangle (well, Layout::scopeWellRadius);

    // `inset 0 2px 8px rgba(0,0,0,.9)` - a short falloff from the top edge, which is what a 2 px
    // offset with an 8 px spread resolves to at this size. Drawn before the grid so the grid reads
    // through it rather than under it.
    {
        juce::ColourGradient recess (Colour::scopeWellRecess, well.getCentreX(), well.getY(),
                                      Colour::scopeWellRecess.withAlpha (0.0f),
                                      well.getCentreX(), well.getY() + 10.0f, false);
        g.setGradientFill (recess);
        g.fillRect (well.withHeight (10.0f));
    }

    // §5's grid, across the whole well: 50 px horizontally, 46 px vertically. Two spacings, not
    // one - the constant this replaced could only ever draw a square grid.
    g.saveState();
    g.reduceClipRegion (well.getSmallestIntegerContainer());
    g.setColour (Colour::scopeGrid);
    for (float x = well.getRight() - gridScrollPhase; x >= well.getX(); x -= Layout::scopeGridSpacingX)
        g.drawVerticalLine ((int) x, well.getY(), well.getBottom());
    for (float y = well.getY(); y <= well.getBottom(); y += Layout::scopeGridSpacingY)
        g.drawHorizontalLine ((int) y, well.getX(), well.getRight());
    g.restoreState();

    g.setColour (Colour::scopeGutterRule);
    g.drawVerticalLine ((int) gutter.getX(), well.getY(), well.getBottom());

    g.saveState();
    g.reduceClipRegion (plot.getSmallestIntegerContainer());

    g.setColour (Colour::scopeBaseline);
    g.drawHorizontalLine ((int) baselineY, plot.getX() + Layout::scopeTraceInsetX,
                           plot.getRight() - Layout::scopeTraceInsetX);

    // This frame's visible columns, oldest -> newest, left -> right, off the history ring.
    const int visibleColumns = juce::jmin (historySize,
        (int) std::ceil (plot.getWidth() / Layout::scopePixelsPerFrame) + 1);

    juce::Path envelopePath, fillPath;
    bool firstColumn = true;
    float lastX = plot.getRight();

    for (int col = 0; col < visibleColumns; ++col)
    {
        const int age = visibleColumns - 1 - col;                 // 0 = newest, rightmost
        const int idx = ((writeIndex - 1 - age) % historySize + historySize) % historySize;
        const float x = plot.getRight() - (float) age * Layout::scopePixelsPerFrame;
        if (x < plot.getX() - Layout::scopePixelsPerFrame)
            continue;

        // Grey input underlay - 1 px vertical strokes, grey, never the accent. §7 allows exactly
        // one accent on this panel and it is the trace, the lamp and its glow.
        const float triggerY = baselineY - triggerDisplayHistory[(size_t) idx] * (baselineY - ceilingY);
        g.setColour (Colour::scopeInputWaveform);
        g.drawVerticalLine ((int) x, triggerY, baselineY);

        const float envY = baselineY - envelopeHistory[(size_t) idx] * (baselineY - ceilingY);
        if (firstColumn)
        {
            envelopePath.startNewSubPath (x, envY);
            fillPath.startNewSubPath (x, baselineY);
            fillPath.lineTo (x, envY);
            firstColumn = false;
        }
        else
        {
            envelopePath.lineTo (x, envY);
            fillPath.lineTo (x, envY);
        }
        lastX = x;
    }

    if (! firstColumn)
    {
        fillPath.lineTo (lastX, baselineY);
        fillPath.closeSubPath();

        juce::ColourGradient fillGradient (Colour::scopeFillTop, plot.getCentreX(), ceilingY,
                                            Colour::scopeFillBottom, plot.getCentreX(), baselineY, false);
        g.setGradientFill (fillGradient);
        g.fillPath (fillPath);

        // `drop-shadow(0 0 6px rgba(255,43,28,.55))`. Blurring the raw centreline gives nothing -
        // a path with no fillable area casts no shadow - so the stroked OUTLINE is what is blurred.
        juce::Path strokeOutline;
        juce::PathStrokeType (Layout::scopeTraceThickness, juce::PathStrokeType::mitered,
                               juce::PathStrokeType::butt).createStrokedPath (strokeOutline, envelopePath);
        juce::DropShadow glow (Colour::gateAccent.withAlpha (0.55f), 6, {0, 0});
        glow.drawForPath (g, strokeOutline);

        g.setColour (Colour::gateAccent);
        g.strokePath (envelopePath, juce::PathStrokeType (Layout::scopeTraceThickness,
                                                           juce::PathStrokeType::mitered,
                                                           juce::PathStrokeType::butt));
    }

    g.restoreState();

    /*  The legends, OUTSIDE the plot clip - that separation is why the gutter exists. Barlow
        Condensed 10 / 13 at `#9aa1a6`, which is §7's 7.48:1 on this glass; it was Share Tech Mono
        at `#A0B2BA` against a gradient ground that no longer exists.

        **`−∞` is built from U+2212 MINUS SIGN and U+221E INFINITY**, never written as a literal:
        `juce::String`'s `const char*` constructor decodes Latin-1, not UTF-8. */
    const auto legendFont = labelFont (labelFontHeightForCssPx (Layout::scopeLegendCssPx));

    drawTrackedText (g, "GATE ENV", legendFont,
                      trackingPxForEm (Layout::scopeTitleTrackingEm, Layout::scopeLegendCssPx),
                      juce::Rectangle<float> (well.getX() + Layout::scopeTitleLocalX,
                                               well.getY() + Layout::scopeTitleLocalY,
                                               200.0f, Layout::scopeLegendLineBox),
                      juce::Justification::centredLeft, Colour::scopeAnnotation);

    /*  §8's scope-legend row is "500/600", and the split is the suite's own hierarchy: `GATE ENV`
        is a title and keeps `legendFont` at 600, while the scale VALUES below take 500. The two
        trackings say the same thing — .20 em on the title, .14 on the values.  */
    const auto scaleFont = numeralFont (labelFontHeightForCssPx (Layout::scopeLegendCssPx));
    const auto scaleTracking = trackingPxForEm (Layout::scopeScaleTrackingEm, Layout::scopeLegendCssPx);
    const float legendRight = gutter.getRight() - Layout::scopeLegendRightInset;

    drawTrackedText (g, "0 dB", scaleFont, scaleTracking,
                      juce::Rectangle<float> (gutter.getX(), well.getY() + Layout::scopeTopLegendLocalY,
                                               legendRight - gutter.getX(), Layout::scopeLegendLineBox),
                      juce::Justification::centredRight, Colour::scopeAnnotation);

    const juce::String negativeInfinity =
        juce::String::charToString ((juce::juce_wchar) 0x2212)
      + juce::String::charToString ((juce::juce_wchar) 0x221E);

    drawTrackedText (g, negativeInfinity, scaleFont, scaleTracking,
                      juce::Rectangle<float> (gutter.getX(), well.getY() + Layout::scopeBottomLegendLocalY,
                                               legendRight - gutter.getX(), Layout::scopeLegendLineBox),
                      juce::Justification::centredRight, Colour::scopeAnnotation);

    // §5's frame, stroked last so nothing is drawn over it.
    g.setColour (Colour::scopeWellRing);
    g.drawRoundedRectangle (well.reduced (0.5f), Layout::scopeWellRadius, 1.0f);
}
