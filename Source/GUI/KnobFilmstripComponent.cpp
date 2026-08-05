#include "KnobFilmstripComponent.h"

KnobFilmstripComponent::KnobFilmstripComponent(GatecrasherTheme::KnobFilmstripSize size, float diameterPx,
                                                 float tickSpacingDegreesIn, bool isAlgorithmSelector)
    : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox),
      filmstripSize(size), diameter(diameterPx), tickSpacingDegrees(tickSpacingDegreesIn),
      algorithmSelectorTicks(isAlgorithmSelector)
{
    setRotaryParameters(juce::degreesToRadians(GatecrasherTheme::Layout::knobArcStartDegrees),
                         juce::degreesToRadians(GatecrasherTheme::Layout::knobArcEndDegrees), true);
}

void KnobFilmstripComponent::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float radius = diameter * 0.5f;

    // Filmstrip frame - drawn into the knob's full bounding box (diameter + ~7% bleed), not just
    // the knob circle, since the cast shadow is baked in and bleeds outside the circle (section 4).
    // Blitted straight onto the bare chassis with normal alpha blending, no backdrop underneath:
    // while the background was the fully dressed render this had to contend with a baked knob at
    // this same position, whose frozen pointer showed through the frame's own semi-transparency as a
    // second, wrong-angle needle. Everything that used to be needed to suppress that (erasing the
    // background, then filling a colour sampled off the strip itself) is gone with the baked knob.
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

    // Tick ring - not part of the filmstrip (it doesn't rotate with the knob). Drawn LAST, on top of
    // the backdrop fill and the filmstrip frame, rather than first: the fill above is opaque and (on
    // the larger knobs, whose knobBoundingBoxBleed margin is a bigger absolute distance) reaches out
    // far enough to swallow the inner end of these low-alpha ticks, which read as missing entirely
    // rather than just losing the intended "frame's baked shadow overlaps the tick's inner edge"
    // blend. Drawing last costs that one cosmetic nuance but keeps them visible at every knob size.
    if (algorithmSelectorTicks)
    {
        // Section 3: "Algorithm selector instead gets 4 ticks at 45, 135, 225, 315 in #3F454A
        // @70%" - a fixed decorative diamond rather than the regular swept ring.
        for (const float angle : {45.0f, 135.0f, 225.0f, 315.0f})
        {
            const auto inner = pointOnCircle(centre, radius + Layout::tickInnerOffset, angle);
            const auto outer = pointOnCircle(centre, radius + Layout::tickOuterOffset, angle);
            g.setColour(Colour::tickMark.withAlpha(0.7f));
            g.drawLine({inner, outer}, 1.0f);
        }
    }
    else
    {
        const int tickCount = tickCountForSpacing(tickSpacingDegrees);
        for (int i = 0; i < tickCount; ++i)
        {
            const float angle = Layout::knobArcStartDegrees
                + (float) i / (float) (tickCount - 1)
                  * (Layout::knobArcEndDegrees - Layout::knobArcStartDegrees);
            const auto inner = pointOnCircle(centre, radius + Layout::tickInnerOffset, angle);
            const auto outer = pointOnCircle(centre, radius + Layout::tickOuterOffset, angle);
            g.setColour(Colour::tickMark.withAlpha(0.5f));
            g.drawLine({inner, outer}, 1.0f);
        }
    }
}
