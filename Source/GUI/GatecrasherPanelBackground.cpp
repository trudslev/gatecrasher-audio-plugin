#include "GatecrasherPanelBackground.h"
#include "GatecrasherTheme.h"

GatecrasherPanelBackground::GatecrasherPanelBackground()
{
    // Pure background - never intercepts mouse input, so every real control layered on top of it
    // (including ones that happen to sit within its bounds) still receives clicks normally.
    setInterceptsMouseClicks(false, false);
}

/*  **This was one drawImage of a 1920 x 868 plate and is now drawn.** §0: no plate, no filmstrips,
    no bitmap of any panel element. The fascia is a 2 px procedural repeat with nothing that wants
    baking, so call 6's per-casting permission applies and nothing exports at 3x; if a wear layer is
    ever added it becomes a plate and the call binds.

    The chassis is four objects, and they are four rows of this casting's enumeration rather than a
    list anyone wrote here: the fascia, two rails, four screws, three dividers. Regenerate it with

        python3 ../tools/enumerate_prototype.py "design/Gatecrasher GR-85 Panel.dc.html" \
            --canvas 1340x700

    which walks the delivered prototype rather than being maintained by hand - the reason being that
    Chorus-60's hand-authored plate enumeration came back thirteen rows short with every row in it
    ink, and this casting has far more to draw than that one did.  */
void GatecrasherPanelBackground::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const auto w = Layout::canvasWidth, h = Layout::canvasHeight;

    /*  Fascia: `repeating-linear-gradient(90deg, #c3c8cc 0 2px, #bdc2c7 2px 4px)`.

        Drawn as hard 2 px stripes rather than as a JUCE gradient, because a `ColourGradient` would
        interpolate between the two and the CSS does not - the stops are `0 2px` and `2px 4px`, so
        each colour holds flat across its own two pixels. An interpolated version reads as a wash at
        1:1 and as a moire against the display grid when scaled, which is the failure the "embed 2x
        only" rule records for fine fascia texture. */
    g.setColour(Colour::fasciaLight);
    g.fillRect(0.0f, 0.0f, w, h);
    g.setColour(Colour::fasciaDark);
    for (float x = Layout::fasciaStripeW; x < w; x += Layout::fasciaStripeW * 2.0f)
        g.fillRect(x, 0.0f, Layout::fasciaStripeW, h);

    // Rack rails, §1: mirrored gradients, so the highlight runs toward the panel on both sides.
    {
        juce::ColourGradient left(Colour::railEdge, 0.0f, 0.0f,
                                   Colour::railEdgeFar, Layout::railW, 0.0f, false);
        left.addColour(0.45, Colour::railHighlight);
        g.setGradientFill(left);
        g.fillRect(0.0f, 0.0f, Layout::railW, h);

        juce::ColourGradient right(Colour::railEdgeFar, w - Layout::railW, 0.0f,
                                    Colour::railEdge, w, 0.0f, false);
        right.addColour(0.55, Colour::railHighlight);
        g.setGradientFill(right);
        g.fillRect(w - Layout::railW, 0.0f, Layout::railW, h);
    }

    // Four Ø11 screws. Positioned from the canvas rather than from a table of four points, so the
    // right-hand pair cannot drift from the left when the canvas moves.
    for (const float cx : { Layout::screwInset,
                             w - Layout::screwInset - Layout::screwDiameter })
        for (const float cy : { Layout::screwTopY, Layout::screwBottomY })
        {
            const juce::Rectangle<float> r(cx, cy, Layout::screwDiameter, Layout::screwDiameter);
            juce::ColourGradient head(Colour::screwHighlight,
                                       r.getX() + r.getWidth() * 0.38f,
                                       r.getY() + r.getHeight() * 0.30f,
                                       Colour::screwShadow, r.getRight(), r.getBottom(), true);
            head.addColour(0.60, Colour::screwBody);
            g.setGradientFill(head);
            g.fillEllipse(r);
        }

    // §1's three column dividers.
    g.setColour(Colour::columnDivider);
    for (const float x : Layout::dividerX)
        g.fillRect(x, Layout::dividerY, 1.0f, Layout::dividerH);

    paintPrintedLabels(g);
}

/*  **Every printed label on the fascia, drawn in one pass.** All of it was pixels in the plate.

    The three tables are walked rather than called site by site, which makes a heading that moves a
    geometry question with one answer instead of eight places to keep in step - the same move as
    filtering one enumerable list by containment rather than holding three lists of names.

    **`StateLabels` is DELETED by this, not ported.** It drew eight of these - the four shoe legends
    and the four corner labels - and indicated state by FONT WEIGHT, 700 for the selected half
    against 400 for the other. §6, §8.1 and §8.3 each forbid exactly that, and §8.1 states the
    general form: *ink weight never stands in for illumination.* The shoe's lit half and the
    selector's pointer carry the state now; the ink does not move, change weight or redraw. */
void GatecrasherPanelBackground::paintPrintedLabels(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const auto draw = [&g](const Layout::PrintedLabel& l, float cssPx, float lineBox,
                            float trackingEm, juce::Colour ink)
    {
        drawTrackedText(g, l.text, labelFont(labelFontHeightForCssPx(cssPx)),
                         trackingPxForEm(trackingEm, cssPx),
                         juce::Rectangle<float>(l.x, l.y, l.width, lineBox),
                         juce::Justification(l.justify), ink);
    };

    for (const auto& l : Layout::sectionHeadings)
        draw(l, Layout::sectionHeadingCssPx, Layout::sectionHeadingLineBox,
              Layout::sectionHeadingTrackingEm, Colour::panelInk);

    for (const auto& l : Layout::cornerLabels)
        draw(l, Layout::cornerLabelCssPx, Layout::cornerLabelLineBox,
              Layout::cornerLabelTrackingEm, Colour::panelInk);

    for (const auto& l : Layout::shoeLegends)
        draw(l, Layout::shoeLegendCssPx, Layout::shoeLegendLineBox,
              Layout::shoeLegendTrackingEm, Colour::panelInk);

    draw({"IN", Layout::meterCaptionX, Layout::meterCaptionY, Layout::meterCaptionW,
           juce::Justification::centred},
          Layout::meterCaptionCssPx, Layout::meterCaptionLineBox,
          Layout::meterCaptionTrackingEm, Colour::panelInk);

    draw({"GATE OPEN", Layout::gateOpenLegendX, Layout::gateOpenLegendY, 200.0f,
           juce::Justification::centredLeft},
          Layout::gateOpenLegendCssPx, Layout::gateOpenLegendLineBox,
          Layout::gateOpenLegendTrackingEm, Colour::panelInk);

    // Its own ink and its own tracking - §7 names it "scope header data", and §9 gained the row for
    // it in export 6 after the prototype enumeration found the type table a row short.
    draw({Layout::scopeHeaderText, Layout::scopeHeaderX, Layout::scopeHeaderY, Layout::scopeHeaderW,
           juce::Justification::centredRight},
          Layout::scopeHeaderCssPx, Layout::scopeHeaderLineBox,
          Layout::scopeHeaderTrackingEm, Colour::scopeHeaderInk);
}
