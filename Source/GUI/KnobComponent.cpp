#include "KnobComponent.h"

using namespace GatecrasherTheme;

KnobComponent::KnobComponent (const Layout::KnobSpec& spec, const Layout::KnobScale& scale)
    : knobSpec (spec), knobScale (scale)
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setRotaryParameters (juce::degreesToRadians (Layout::knobArcStartDegrees) + juce::MathConstants<float>::pi,
                         juce::degreesToRadians (Layout::knobArcEndDegrees) + juce::MathConstants<float>::pi,
                         true);
    setMouseDragSensitivity (Layout::knobDragPixels);
}

bool KnobComponent::hitTest (int x, int y)
{
    const float r = knobSpec.diameter * 0.5f + Layout::knobClickMargin;
    const auto c = getLocalBounds().toFloat().getCentre();
    return juce::Point<float> ((float) x, (float) y).getDistanceFrom (c) <= r;
}

/*  Everything except the pointer, rendered once per device scale.

    Drawn into an image at the device's own pixel density rather than at logical size, so the ticks
    and the 1.4 px arc stay hairlines on a retina display instead of being blitted up from a 1x
    render. That is the same reason the suite embeds artwork at 2x and scales down.  */
void KnobComponent::renderStaticLayer (float deviceScale)
{
    const auto bounds = getLocalBounds().toFloat();
    staticLayer = juce::Image (juce::Image::ARGB,
                                juce::roundToInt (bounds.getWidth()  * deviceScale),
                                juce::roundToInt (bounds.getHeight() * deviceScale), true);

    juce::Graphics g (staticLayer);
    g.addTransform (juce::AffineTransform::scale (deviceScale));

    const auto centre = bounds.getCentre();
    const float r = knobSpec.diameter * 0.5f;

    /*  §3's sweep arc: a 270 deg wedge at rgba(22,25,28,.34), masked to a 1.4 px ring at r + 6.
        Drawn as a stroked arc rather than as a conic gradient masked to a ring - the CSS says conic
        because that is how a browser makes a wedge, and the wedge is uniform, so what it resolves to
        IS an arc. Sweeping it by hand would be reproducing the mechanism rather than the mark.  */
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y,
                            r + Layout::knobArcRadiusGap, r + Layout::knobArcRadiusGap, 0.0f,
                            juce::degreesToRadians (Layout::knobArcStartDegrees),
                            juce::degreesToRadians (Layout::knobArcEndDegrees), true);
        g.setColour (Colour::knobSweepArc);
        g.strokePath (arc, juce::PathStrokeType (Layout::knobArcThickness));
    }

    /*  Ticks. **Both weights share an inner end at r + 8**; what differs is the outer. Asserting
        that shared end is the arm worth having - two lengths only restate themselves.  */
    for (int i = 0; i < knobScale.markCount; ++i)
    {
        const auto& mark = knobScale.marks[(size_t) i];
        const float ink   = mark.numbered ? Layout::knobMajorTickInk   : Layout::knobMinorTickInk;
        const float width = mark.numbered ? Layout::knobMajorTickWidth : Layout::knobMinorTickWidth;
        const float inner = r + Layout::knobTickInnerGap;

        juce::Path tick;
        tick.addRectangle (-width * 0.5f, -(inner + ink), width, ink);
        g.setColour (Colour::panelInk);
        g.fillPath (tick, juce::AffineTransform::rotation (juce::degreesToRadians (mark.angleDeg))
                              .translated (centre.x, centre.y));
    }

    /*  Numerals, placed by rotation fraction and counter-rotated to upright. §3.3's selector prints
        none - it carries corner labels instead, which are in the panel's printed layer.  */
    // §8's scale numeral is Barlow Condensed 500 — a value, not a name.
    const auto numeralFont = GatecrasherTheme::numeralFont (labelFontHeightForCssPx (Layout::knobNumeralCssPx));
    const float numeralTracking = trackingPxForEm (Layout::knobNumeralTrackingEm, Layout::knobNumeralCssPx);
    const float numeralRadius = Layout::knobNumeralRadius (r);

    for (int i = 0; i < knobScale.markCount; ++i)
    {
        const auto& mark = knobScale.marks[(size_t) i];
        if (! mark.numbered || juce::String (mark.numeral).isEmpty())
            continue;

        const float radians = juce::degreesToRadians (mark.angleDeg);
        const juce::Point<float> at (centre.x + std::sin (radians) * numeralRadius,
                                      centre.y - std::cos (radians) * numeralRadius);

        /*  **`CharPointer_UTF8`, and this line is here because it was WRONG first.** The table
            stores U+2212 as UTF-8 bytes in a narrow literal, and `juce::String`'s `const char*`
            constructor decodes **Latin-1** — so the first build drew `â^'60` where §3.2 says `−60`.

            Every other above-ASCII glyph on this panel is built from its codepoint for exactly this
            reason, and the rule is written three times in this repo including in the comment above
            the very table that broke it. **A rule is not a mechanism.** What stops it recurring is
            that every numeral now goes through one call site, not that the trap is documented
            again.  */
        drawTrackedText (g, juce::String (juce::CharPointer_UTF8 (mark.numeral)),
                          numeralFont, numeralTracking,
                          juce::Rectangle<float> (at.x - 30.0f, at.y - Layout::knobNumeralLineBox * 0.5f,
                                                   60.0f, Layout::knobNumeralLineBox),
                          juce::Justification::centred, Colour::panelInk);
    }

    /*  §8's UNIT row, in the arc's bottom gap — **drawn for the first time**.

        Every one of the fifteen constants this needs already existed and was correct; nothing
        consumed any of them. `knobScales` carries a `unit` string per knob, `knobUnitCssPx`,
        `knobUnitLineBox` and `knobUnitTrackingEm` carry §8's 10 / 13 / .16 em, and
        `knobUnitTop(d)` carries §3.1's registration — *unit and legend are positioned off a Ø76 box
        for EVERY class*, so the offset is `d + 20 + (76 − d)/2`. That evaluates to **96 at Ø76 and
        86 at Ø56**, which is where the delivered prototype puts all eleven units, to the pixel.

        **Eleven of fifteen rings print one and four do not, and the split is already in the table.**
        The four silent ones are exactly the four with no unit to print: ALGORITHM is a detented
        selector with no numerals, and SIZE, HF and LF are unitless 0–1.0 controls. So §8's row is
        owed rather than aspirational and needed no qualifying — it is implementable as written, and
        `knobScale.unit` is its own data source.

        **None of this panel's numerals carries its own unit**, which is what makes the row real
        work rather than a double-print. Elmer is the contrast and its split is genuine: RATIO
        prints `4:1` and RELEASE prints `0.6s` and `AUTO`, values that carry their own suffixes, so
        those two rings correctly hold a null unit. Gatecrasher has no such ring — `1k` and `20k`
        are magnitude prefixes on a Hz scale, not units, and the prototype prints `Hz` under that
        ring anyway.  */
    if (const auto* unit = knobScale.unit; unit != nullptr && *unit != '\0')
    {
        const auto unitFont = GatecrasherTheme::numeralFont (labelFontHeightForCssPx (Layout::knobUnitCssPx));
        const float top = (centre.y - r) + Layout::knobUnitTop (knobSpec.diameter);

        drawTrackedText (g, juce::String (juce::CharPointer_UTF8 (unit)), unitFont,
                          trackingPxForEm (Layout::knobUnitTrackingEm, Layout::knobUnitCssPx),
                          juce::Rectangle<float> (centre.x - 30.0f, top, 60.0f, Layout::knobUnitLineBox),
                          juce::Justification::centred, Colour::panelInk);
    }

    // The body, over the ring: `0 3px 6px rgba(0,0,0,.38)` under it, then the radial face.
    const juce::Rectangle<float> cap (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);

    g.setColour (Colour::knobBodyDrop);
    g.fillEllipse (cap.translated (0.0f, 3.0f));

    juce::ColourGradient face (Colour::knobBodyHighlight,
                                cap.getX() + cap.getWidth() * 0.38f,
                                cap.getY() + cap.getHeight() * 0.26f,
                                Colour::knobBodyEdge, cap.getRight(), cap.getBottom(), true);
    face.addColour (0.46, Colour::knobBodyMid);
    g.setGradientFill (face);
    g.fillEllipse (cap);

    // `inset 0 -3px 8px rgba(0,0,0,.65)` then `inset 0 1px 0 rgba(255,255,255,.18)`: floor first,
    // lip last, both clipped to the cap so neither spills onto the ring behind it.
    {
        juce::Graphics::ScopedSaveState insetState (g);
        juce::Path capPath;
        capPath.addEllipse (cap);
        g.reduceClipRegion (capPath);

        juce::ColourGradient floorShadow (Colour::knobBodyFloor.withAlpha (0.0f),
                                           centre.x, cap.getBottom() - 11.0f,
                                           Colour::knobBodyFloor, centre.x, cap.getBottom(), false);
        g.setGradientFill (floorShadow);
        g.fillEllipse (cap);

        g.setColour (Colour::knobBodyLip);
        g.fillRect (cap.getX(), cap.getY() + 1.0f, cap.getWidth(), 1.0f);
    }
}

void KnobComponent::paint (juce::Graphics& g)
{
    const float deviceScale = g.getInternalContext().getPhysicalPixelScaleFactor();

    if (staticLayer.isNull() || std::abs (deviceScale - cachedDeviceScale) > 1.0e-3f)
    {
        renderStaticLayer (deviceScale);
        cachedDeviceScale = deviceScale;
    }

    g.drawImage (staticLayer, getLocalBounds().toFloat(),
                  juce::RectanglePlacement::stretchToFit);

    // The pointer, live. §3: 3 x 31 on Ø76 and 3 x 21 on Ø56, which is r - 7 either way.
    const auto centre = getLocalBounds().toFloat().getCentre();
    const float r = knobSpec.diameter * 0.5f;
    const float length = r - Layout::knobPointerInset;
    const float value01 = (float) valueToProportionOfLength (getValue());

    juce::Path pointer;
    pointer.addRoundedRectangle (-Layout::knobPointerWidth * 0.5f, -length,
                                  Layout::knobPointerWidth, length,
                                  Layout::knobPointerWidth * 0.5f);
    g.setColour (Colour::knobPointer);
    g.fillPath (pointer, juce::AffineTransform::rotation (
                    juce::degreesToRadians (knobAngleForValue01 (value01)))
                        .translated (centre.x, centre.y));
}
