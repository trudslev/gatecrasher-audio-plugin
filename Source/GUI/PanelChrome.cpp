#include "PanelChrome.h"
#include "GatecrasherTheme.h"

using namespace GatecrasherTheme;

namespace
{
    // Every engraved string on the panel is drawn the same way: pick the spec's CSS size, convert it
    // to a juce::Font height, convert the CSS em tracking to absolute px at that size, and centre it
    // on a point rather than inside a box (the artwork centres labels on their control's axis, and a
    // centre-point is what every measurement taken off the dressed render actually recorded).
    void drawCentred(juce::Graphics& g, const juce::String& text, float centreX, float centreY,
                      float cssPx, float trackingEm, juce::Colour colour)
    {
        const auto font = labelFont(labelFontHeightForCssPx(cssPx));
        const float tracking = trackingPxForEm(trackingEm, cssPx);
        const float width = trackedTextWidth(text, font, tracking);

        drawTrackedText(g, text, font, tracking,
                         {centreX - width * 0.5f, centreY - cssPx, width, cssPx * 2.0f},
                         juce::Justification::centred, colour);
    }

    void drawLeftAligned(juce::Graphics& g, const juce::String& text, float leftX, float centreY,
                          float cssPx, float trackingEm, juce::Colour colour)
    {
        const auto font = labelFont(labelFontHeightForCssPx(cssPx));
        const float tracking = trackingPxForEm(trackingEm, cssPx);
        const float width = trackedTextWidth(text, font, tracking);

        drawTrackedText(g, text, font, tracking, {leftX, centreY - cssPx, width, cssPx * 2.0f},
                         juce::Justification::centred, colour);
    }

    // A recessed LED window: 1px border with the dark interior inside it. The spec's inset shadow is
    // approximated by a single darker line under the top edge - at 25px tall a full inner gradient
    // reads as noise rather than depth.
    void drawRecessedWindow(juce::Graphics& g, juce::Rectangle<float> rect, juce::Colour border)
    {
        g.setColour(Colour::ledWindowBg);
        g.fillRect(rect);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRect(rect.getX() + 1.0f, rect.getY() + 1.0f, rect.getWidth() - 2.0f, 1.5f);
        g.setColour(border);
        g.drawRect(rect, 1.0f);
    }
}

PanelChrome::PanelChrome()
{
    // Pure decoration between the background and every real control - must never swallow a click.
    setInterceptsMouseClicks(false, false);
}

void PanelChrome::paint(juce::Graphics& g)
{
    paintHeader(g);
    paintSectionHeadings(g);
    paintKnobLabels(g);
    paintSwitchCaptions(g);
    paintInputMeterFrame(g);
}

void PanelChrome::paintHeader(juce::Graphics& g)
{
    // Nameplate subtitle, left-aligned beside the wordmark (WordmarkComponent draws the wordmark
    // itself). Two lines, the model line deliberately a shade lighter than the descriptor above it.
    drawLeftAligned(g, "GATED AMBIENCE PROCESSOR", Layout::subtitleX, Layout::subtitleLine1CentreY,
                     9.0f, 0.26f, Colour::engravedHeading);
    // A plain hyphen, not the mockup's non-breaking one: Barlow Condensed has no U+2011 glyph, so
    // that character renders as a tofu box. Nothing here wraps, so the distinction is moot anyway.
    drawLeftAligned(g, juce::String(juce::CharPointer_UTF8("MODEL GR-85 \xc2\xb7 STEREO")),
                     Layout::subtitleX, Layout::subtitleLine2CentreY, 9.0f, 0.26f, Colour::subtitleSecondary);

    // Left-aligned with the program window it captions.
    drawLeftAligned(g, "PROGRAM", Layout::programWindowX, Layout::headerCaptionCentreY,
                     9.0f, 0.24f, Colour::headerCaption);

    // Program window, plus the 1px rule splitting its FACT/USER tag cell from the name cell.
    // ProgramHeader fills and writes into both cells; only the frame belongs here.
    const juce::Rectangle<float> programWindow(Layout::programWindowX, Layout::programWindowY,
                                                Layout::programWindowW, Layout::programWindowH);
    drawRecessedWindow(g, programWindow, Colour::ledWindowBorder);
    g.setColour(Colour::programCellDivider);
    g.fillRect(Layout::programNameCellX - 1.0f, programWindow.getY() + 1.0f, 1.0f,
                programWindow.getHeight() - 2.0f);

    const juce::Rectangle<float> inWindow(Layout::inWindowX, Layout::inWindowY,
                                           Layout::inWindowW, Layout::inWindowH);
    const juce::Rectangle<float> outWindow(Layout::outWindowX, Layout::outWindowY,
                                            Layout::outWindowW, Layout::outWindowH);
    drawRecessedWindow(g, inWindow, Colour::ledWindowBorder);
    drawRecessedWindow(g, outWindow, Colour::ledWindowBorder);

    drawCentred(g, "IN", inWindow.getCentreX(), Layout::headerCaptionCentreY, 9.0f, 0.20f,
                 Colour::headerCaption);
    drawCentred(g, "OUT", outWindow.getCentreX(), Layout::headerCaptionCentreY, 9.0f, 0.20f,
                 Colour::headerCaption);
}

void PanelChrome::paintSectionHeadings(juce::Graphics& g)
{
    drawCentred(g, "INPUT / TRIGGER", Layout::leftColCentreX, Layout::sectionHeadingCentreY,
                 9.5f, 0.28f, Colour::engravedHeading);
    drawCentred(g, "REVERB TANK", Layout::reverbColCentreX, Layout::sectionHeadingCentreY,
                 9.5f, 0.28f, Colour::engravedHeading);
    drawCentred(g, "OUTPUT", Layout::outputColCentreX, Layout::sectionHeadingCentreY,
                 9.5f, 0.28f, Colour::engravedHeading);

    drawCentred(g, "TRIGGER FILTER", Layout::leftColCentreX, Layout::triggerFilterLabelCentreY,
                 9.0f, 0.20f, Colour::tertiaryGroupLabel);
    drawCentred(g, "TANK DAMPING", Layout::reverbColCentreX, Layout::tankDampingLabelCentreY,
                 9.0f, 0.20f, Colour::tertiaryGroupLabel);

    drawCentred(g, "v1.0", Layout::outputColCentreX, Layout::versionCentreY, 9.0f, 0.18f,
                 Colour::versionText);

    // Scope's timebase annotation, right-aligned to the scope's own right edge. Drawn as two runs
    // with a fixed gap, matching the mockup, rather than one string with padding - the gap there is
    // a flex gap, not spaces.
    const auto annotationFont = monoFont(monoFontHeightForCssPx(9.0f));
    const float annotationTracking = trackingPxForEm(0.10f, 9.0f);
    const juce::String divisions = "50 ms / DIV";
    const float divisionsWidth = trackedTextWidth(divisions, annotationFont, annotationTracking);
    const float divisionsLeft = Layout::envelopeAnnotationRight - divisionsWidth;

    drawTrackedText(g, divisions, annotationFont, annotationTracking,
                     {divisionsLeft, Layout::envelopeAnnotationCentreY - 9.0f, divisionsWidth, 18.0f},
                     juce::Justification::centred, Colour::headerCaption);

    const juce::String envelope = "ENVELOPE";
    const float envelopeWidth = trackedTextWidth(envelope, annotationFont, annotationTracking);
    drawTrackedText(g, envelope, annotationFont, annotationTracking,
                     {divisionsLeft - Layout::envelopeAnnotationGap - envelopeWidth,
                      Layout::envelopeAnnotationCentreY - 9.0f, envelopeWidth, 18.0f},
                     juce::Justification::centred, Colour::headerCaption);
}

void PanelChrome::paintKnobLabels(juce::Graphics& g)
{
    for (const auto& spec : Layout::knobs)
    {
        if (spec.label == nullptr)
            continue; // algorithm selector - labelled by its four corner labels instead

        // A/H/R are both larger and darker than the rest, which is what makes the gate's own
        // timing controls read as the panel's primary row.
        const bool isAhr = spec.labelCssPx > 9.5f;
        drawCentred(g, spec.label, spec.cx, spec.labelCentreY, spec.labelCssPx, spec.labelTrackingEm,
                     isAhr ? Colour::ahrLabelText : Colour::controlLabelText);
    }
}

void PanelChrome::paintSwitchCaptions(juce::Graphics& g)
{
    drawCentred(g, "KEY SOURCE", Layout::keySourceTrackX + Layout::switchTrackW * 0.5f,
                 Layout::keySourceTrackY - Layout::switchCaptionCentreAboveTrack,
                 9.0f, 0.20f, Colour::tertiaryGroupLabel);
    drawCentred(g, "SHAPE", Layout::shapeTrackX + Layout::switchTrackW * 0.5f,
                 Layout::shapeTrackY - Layout::switchCaptionCentreAboveTrack,
                 9.0f, 0.20f, Colour::tertiaryGroupLabel);
}

void PanelChrome::paintInputMeterFrame(juce::Graphics& g)
{
    const juce::Rectangle<float> frame(Layout::meterFrameX, Layout::meterFrameY,
                                        Layout::meterFrameW, Layout::meterFrameH);
    drawRecessedWindow(g, frame, Colour::meterFrameBorder);

    drawCentred(g, "IN", frame.getCentreX(), Layout::meterCaptionCentreY, 9.0f, 0.18f,
                 Colour::valueText);
}
