#pragma once

#include "../PluginProcessor.h"
#include "GatecrasherTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
    The eight state-dependent labels of spec section 0.4 — `INTERNAL` / `SIDECHAIN`, `HARD` / `SOFT`,
    and the four algorithm corners. This is the ONLY text the build draws outside the two LCD
    windows; everything else on the fascia is baked into the plate.

    They are drawn rather than baked because their weight and colour follow a control. Rev 6 baked
    them at their defaults and asked the build to redraw whichever pair had changed, which cannot
    work — baked pixels cannot be un-drawn, so turning a bold baked word dim would have meant
    painting matched fascia over it first. Rev 7 removed them from the plate; bare fascia sits where
    they go, and all eight are drawn every frame. **Do not reintroduce baked copies of these.**

    Sized to the full canvas and drawn in absolute panel coordinates, like the other overlay
    components, and it takes no mouse input at all — the switches and the selector own their own hit
    areas, and a canvas-sized component that intercepted clicks would swallow theirs.

    Polls on a timer rather than listening to parameters: two of the three controls it reflects are
    switches whose component already repaints on change, and the algorithm is a choice parameter a
    host can move without any GUI gesture. A 20 Hz poll costs nothing next to the scope's 60 Hz and
    keeps this free of parameter-listener lifetime concerns.
*/
class StateLabels final : public juce::Component, private juce::Timer
{
public:
    explicit StateLabels(GatecrasherAudioProcessor&);
    ~StateLabels() override;

    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;

    GatecrasherAudioProcessor& processorRef;

    // Mirrors of the three parameters this reflects, so the timer only repaints on a real change.
    int displayedKeySource = -1;
    int displayedShape = -1;
    int displayedAlgorithm = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateLabels)
};
