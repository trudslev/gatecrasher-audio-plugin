#include "PanelReadouts.h"
#include "GatecrasherTheme.h"
#include "../Parameters.h"
#include <cmath>

using namespace GatecrasherTheme;

namespace
{
    juce::String formatValue(float value, Layout::ValueFormat format)
    {
        switch (format)
        {
            case Layout::ValueFormat::decibels1:
                return juce::String(value, 1) + " dB";

            case Layout::ValueFormat::hertzAuto:
                // Past a kilohertz the artwork switches to "6.3 k" - at this size the extra digits
                // of "6300 Hz" don't fit under a 38px knob.
                return value >= 1000.0f ? juce::String(value / 1000.0f, 1) + " k"
                                         : juce::String(juce::roundToInt(value)) + " Hz";

            case Layout::ValueFormat::millisAuto:
                // A decimal only earns its place below a millisecond, where it's the whole value -
                // the artwork shows "0.4 ms" but plain "4 ms" and "165 ms", so the cut is at 1, not
                // at 10.
                return value < 1.0f ? juce::String(value, 1) + " ms"
                                     : juce::String(juce::roundToInt(value)) + " ms";

            case Layout::ValueFormat::percent0:
                return juce::String(juce::roundToInt(value)) + "%";

            case Layout::ValueFormat::plain2:
                return juce::String(value, 2);

            // The "+" is decided from the ROUNDED value, not the raw one - a trim of +0.02 dB
            // formats as "0.0", and "+0.0" claims a boost that isn't being shown.
            case Layout::ValueFormat::signedInt:
            {
                const int rounded = juce::roundToInt(value);
                return (rounded > 0 ? "+" : "") + juce::String(rounded);
            }

            case Layout::ValueFormat::signed1:
            {
                const auto text = juce::String(value, 1);
                return text.startsWith("-") || text.getFloatValue() == 0.0f ? text : "+" + text;
            }

            case Layout::ValueFormat::none:
            default:
                return {};
        }
    }

    // Used both to draw a readout and to invalidate it - generous, since the text is centred and
    // only has to cover the widest value the knob can reach.
    juce::Rectangle<int> readoutBounds(const Layout::KnobSpec& spec)
    {
        constexpr float halfWidth = 46.0f, halfHeight = 10.0f;
        return juce::Rectangle<float>(spec.cx - halfWidth, spec.valueCentreY - halfHeight,
                                       halfWidth * 2.0f, halfHeight * 2.0f)
            .getSmallestIntegerContainer();
    }
}

PanelReadouts::PanelReadouts(GatecrasherAudioProcessor& processor) : processorRef(processor)
{
    setInterceptsMouseClicks(false, false);

    for (size_t i = 0; i < Layout::knobs.size(); ++i)
        displayedValues[i] = currentValueText(i);

    displayedAlgorithm = currentAlgorithmIndex();

    startTimerHz(30);
}

PanelReadouts::~PanelReadouts()
{
    stopTimer();
}

juce::String PanelReadouts::currentValueText(size_t knobIndex) const
{
    const auto& spec = Layout::knobs[knobIndex];
    if (spec.valueFormat == Layout::ValueFormat::none)
        return {};

    // The raw (real-world, un-normalised) value, same source PluginProcessor reads per block.
    if (const auto* raw = processorRef.apvts.getRawParameterValue(spec.paramID))
        return formatValue(raw->load(), spec.valueFormat);

    return {};
}

int PanelReadouts::currentAlgorithmIndex() const
{
    if (const auto* raw = processorRef.apvts.getRawParameterValue(ParamIDs::algorithm))
        return juce::jlimit(0, 3, (int) raw->load());

    return 0;
}

void PanelReadouts::timerCallback()
{
    for (size_t i = 0; i < Layout::knobs.size(); ++i)
    {
        const auto text = currentValueText(i);
        if (text == displayedValues[i])
            continue;

        displayedValues[i] = text;
        repaint(readoutBounds(Layout::knobs[i]));
    }

    // The corner labels only ever change which one is lit, so they repaint as a single block and
    // only when the choice itself changes.
    const int algorithm = currentAlgorithmIndex();
    if (algorithm != displayedAlgorithm)
    {
        displayedAlgorithm = algorithm;
        repaint(juce::Rectangle<float>(Layout::algoLabelLeftX - 6.0f,
                                        Layout::algoLabelTopCentreY - 12.0f,
                                        Layout::algoLabelRightX - Layout::algoLabelLeftX + 12.0f,
                                        Layout::algoLabelBottomCentreY - Layout::algoLabelTopCentreY + 24.0f)
                    .getSmallestIntegerContainer());
    }
}

void PanelReadouts::paint(juce::Graphics& g)
{
    g.setColour(Colour::valueText);
    g.setFont(monoFont(monoFontHeightForCssPx(9.0f)));

    for (size_t i = 0; i < Layout::knobs.size(); ++i)
    {
        if (displayedValues[i].isEmpty())
            continue;

        const auto& spec = Layout::knobs[i];
        g.drawText(displayedValues[i], readoutBounds(spec).toFloat(), juce::Justification::centred, false);
    }

    // ROOM / PLATE / CHMBR / AMBI around the selector, the current one engraved noticeably darker.
    // Choice order is Room, Plate, Chamber, Ambience (Parameters.h); the artwork lays them out
    // clockwise from top-left, so Chamber is bottom-RIGHT and Ambience bottom-left.
    struct CornerLabel { const char* text; int algorithmIndex; float x; float centreY; bool rightAligned; };
    static constexpr CornerLabel corners[] = {
        {"ROOM",  0, Layout::algoLabelLeftX,  Layout::algoLabelTopCentreY,    false},
        {"PLATE", 1, Layout::algoLabelRightX, Layout::algoLabelTopCentreY,    true},
        {"CHMBR", 2, Layout::algoLabelRightX, Layout::algoLabelBottomCentreY, true},
        {"AMBI",  3, Layout::algoLabelLeftX,  Layout::algoLabelBottomCentreY, false},
    };

    const auto cornerFont = labelFont(labelFontHeightForCssPx(9.0f));
    const float cornerTracking = trackingPxForEm(0.14f, 9.0f);

    for (const auto& corner : corners)
    {
        const float width = trackedTextWidth(corner.text, cornerFont, cornerTracking);
        const float left = corner.rightAligned ? corner.x - width : corner.x;

        drawTrackedText(g, corner.text, cornerFont, cornerTracking,
                         {left, corner.centreY - 9.0f, width, 18.0f}, juce::Justification::centred,
                         corner.algorithmIndex == displayedAlgorithm ? Colour::algorithmActive
                                                                      : Colour::algorithmInactive);
    }
}
