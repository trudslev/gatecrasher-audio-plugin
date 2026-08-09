#include "KnobFilmstripComponent.h"

KnobFilmstripComponent::KnobFilmstripComponent(GatecrasherTheme::KnobFilmstripSize size, float diameterPx)
    : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox),
      filmstripSize(size), diameter(diameterPx)
{
    setRotaryParameters(juce::degreesToRadians(GatecrasherTheme::Layout::knobArcStartDegrees),
                         juce::degreesToRadians(GatecrasherTheme::Layout::knobArcEndDegrees), true);
}

void KnobFilmstripComponent::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const auto centre = getLocalBounds().toFloat().getCentre();

    // The filmstrip frame is the whole of this component's output. Drawn into the knob's full
    // bounding box (diameter + ~7% bleed), not just the circle, since the cast shadow is baked into
    // the strip and bleeds outside it (spec section 1.3). Blitted straight onto the plate with
    // normal alpha blending and no backdrop or shadow of our own - the plate carries the recessed
    // well and the strip carries its own shadow.
    const float boxSize = diameter * Layout::knobBoundingBoxBleed;
    const juce::Rectangle<int> box((int) std::round(centre.x - boxSize * 0.5f),
                                    (int) std::round(centre.y - boxSize * 0.5f),
                                    (int) std::round(boxSize), (int) std::round(boxSize));

    const auto& strip = filmstripSize == KnobFilmstripSize::large ? knobLargeFilmstrip() : knobSmallFilmstrip();

    // sliderPos accounts for the parameter's own skew (e.g. Trigger HP/LP's log skew) via the
    // Slider's NormalisableRange, set up by SliderAttachment from the bound RangedAudioParameter -
    // so the knob's physical rotation always matches the parameter's true travel proportion.
    const float sliderPos = (float) valueToProportionOfLength(getValue());
    const int frame = juce::jlimit(0, 127, (int) std::round(sliderPos * 127.0f));

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(strip, box.getX(), box.getY(), box.getWidth(), box.getHeight(),
                0, frame * 128, 128, 128);
}
