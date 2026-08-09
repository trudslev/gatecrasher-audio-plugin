#include "StateLabels.h"
#include "../Parameters.h"

using namespace GatecrasherTheme;

namespace
{
    int choiceIndex(const juce::AudioProcessorValueTreeState& apvts, const char* id)
    {
        if (auto* v = apvts.getRawParameterValue(id))
            return (int) std::round(v->load(std::memory_order_relaxed));
        return 0;
    }
}

StateLabels::StateLabels(GatecrasherAudioProcessor& p) : processorRef(p)
{
    // Pure overlay: the switches and the algorithm knob own their own hit areas, and this spans the
    // whole canvas, so it must not claim any of them.
    setInterceptsMouseClicks(false, false);
    startTimerHz(20);
}

StateLabels::~StateLabels()
{
    stopTimer();
}

void StateLabels::timerCallback()
{
    const int key = choiceIndex(processorRef.apvts, ParamIDs::keySource);
    const int shape = choiceIndex(processorRef.apvts, ParamIDs::shape);
    const int algo = choiceIndex(processorRef.apvts, ParamIDs::algorithm);

    if (key == displayedKeySource && shape == displayedShape && algo == displayedAlgorithm)
        return;

    displayedKeySource = key;
    displayedShape = shape;
    displayedAlgorithm = algo;
    repaint();
}

void StateLabels::paint(juce::Graphics& g)
{
    // Which of the eight are lit. Indices follow Layout::stateLabels: 0/1 KEY SOURCE, 2/3 SHAPE,
    // 4-7 the algorithm corners in PANEL order (ROOM, PLATE, AMBI, CHMBR).
    //
    // The algorithm rows are the one place panel order and parameter order differ. Section 9.1 gives
    // the parameter as 0=Ambience, 1=Room, 2=Plate, 3=Chamber; the corners read clockwise from the
    // upper left as ROOM, PLATE, then AMBI, CHMBR beneath. So this maps parameter index -> label
    // slot explicitly rather than relying on the two happening to line up, because they do not.
    constexpr int algorithmLabelSlotFor[4] = { 6, 4, 5, 7 };   // Ambience, Room, Plate, Chamber

    const int key = displayedKeySource, shape = displayedShape, algo = displayedAlgorithm;
    const int litAlgorithmSlot = algorithmLabelSlotFor[juce::jlimit(0, 3, algo)];

    const float heightPx = labelFontHeightForCssPx(Layout::stateLabelCssPx);
    const float tracking = trackingPxForEm(Layout::stateLabelTrackingEm, Layout::stateLabelCssPx);
    const auto selectedFont = labelFontBold(heightPx);
    const auto unselectedFont = labelFontRegular(heightPx);

    for (size_t i = 0; i < Layout::stateLabels.size(); ++i)
    {
        const auto& spec = Layout::stateLabels[i];

        const bool selected = i == 0 ? key == 0
                            : i == 1 ? key == 1
                            : i == 2 ? shape == 0
                            : i == 3 ? shape == 1
                                     : (int) i == litAlgorithmSlot;

        // Laid out from the LEFT edge and drawn left-justified, never centred on the recorded
        // centreX: a 700-weight word is wider than the same word at 400, so re-centring would slide
        // it sideways every time the control changed (spec section 0.4).
        //
        // The rect is generously tall and wide because drawTrackedText justifies within it; only the
        // left edge and the baseline carry meaning. Ascent is taken off the font so the spec's
        // baseline lands where it says.
        const auto& font = selected ? selectedFont : unselectedFont;
        const juce::Rectangle<float> rect(spec.x, spec.baselineY - font.getAscent(),
                                           200.0f, font.getAscent() + font.getDescent());

        drawTrackedText(g, spec.text, font, tracking, rect, juce::Justification::left,
                         selected ? Colour::labelSelected : Colour::labelUnselected);
    }
}
