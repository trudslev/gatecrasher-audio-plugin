#include "KnobFilmstripComponent.h"

KnobFilmstripComponent::KnobFilmstripComponent(GatecrasherTheme::KnobFilmstripSize size, float diameterPx)
    // **RotaryVerticalDrag, like the other five.** This was the one casting responding to
    // horizontal drag as well, which is a different feel under the same hand and nothing about
    // Gatecrasher's identity argued for it.
    : juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox),
      filmstripSize(size), diameter(diameterPx)
{
    setRotaryParameters(juce::degreesToRadians(GatecrasherTheme::Layout::knobArcStartDegrees),
                         juce::degreesToRadians(GatecrasherTheme::Layout::knobArcEndDegrees), true);

    setMouseDragSensitivity(GatecrasherTheme::Layout::knobDragPixels);
    setVelocityBasedMode(false);
}

void KnobFilmstripComponent::mouseDown(const juce::MouseEvent& e)
{
    // Sensitivity has to be settled BEFORE Slider::mouseDown records its drag anchor: JUCE measures
    // from that anchor and scales by the current sensitivity, so changing it mid-drag rescales the
    // distance already travelled and the value jumps.
    setMouseDragSensitivity(e.mods.isShiftDown() ? GatecrasherTheme::Layout::knobFineDragPixels
                                                 : GatecrasherTheme::Layout::knobDragPixels);

    juce::Slider::mouseDown(e);
}

void KnobFilmstripComponent::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const auto centre = getLocalBounds().toFloat().getCentre();

    // The filmstrip frame is the whole of this component's output. Drawn into the full FRAME box -
    // 1.333 x the cap diameter, spec section 1.3 - not just the circle, since the cast shadow is
    // baked into the strip and needs every pixel of that margin to fade out in. Blitted straight
    // onto the plate with normal alpha blending and no backdrop or shadow of our own: the plate
    // carries the recessed well and the strip carries its own shadow.
    const float boxSize = diameter * Layout::knobBoundingBoxBleed;
    const juce::Rectangle<int> box((int) std::round(centre.x - boxSize * 0.5f),
                                    (int) std::round(centre.y - boxSize * 0.5f),
                                    (int) std::round(boxSize), (int) std::round(boxSize));

    const auto& strip = filmstripSize == KnobFilmstripSize::large ? knobLargeFilmstrip() : knobSmallFilmstrip();

    // sliderPos accounts for the parameter's own skew (e.g. Trigger HP/LP's log skew) via the
    // Slider's NormalisableRange, set up by SliderAttachment from the bound RangedAudioParameter -
    // so the knob's physical rotation always matches the parameter's true travel proportion.
    const float sliderPos = (float) valueToProportionOfLength(getValue());
    constexpr int lastFrame = Layout::knobFilmstripFrameCount - 1;
    const int frame = juce::jlimit(0, lastFrame, (int) std::round(sliderPos * (float) lastFrame));

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    constexpr int framePx = Layout::knobFilmstripFramePx;
    g.drawImage(strip, box.getX(), box.getY(), box.getWidth(), box.getHeight(),
                0, frame * framePx, framePx, framePx);
}

bool KnobFilmstripComponent::hitTest(int x, int y)
{
    // Claim the CAP, not the bounds. The bounds are the frame box, which since Rev 9 reaches .167 of
    // the diameter past the cap so the baked shadow has room to fade - on THRESHOLD that is 10px of
    // transparent margin sitting over the plate's printed "-45" and "-15". Without this, clicking a
    // printed numeral would grab the knob.
    const auto centre = getLocalBounds().toFloat().getCentre();
    const float r = diameter * 0.5f + GatecrasherTheme::Layout::knobClickMargin;
    return centre.getDistanceFrom(juce::Point<float>((float) x, (float) y)) <= r;
}
