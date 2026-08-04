#include "GatecrasherLookAndFeel.h"
#include "GatecrasherTheme.h"

GatecrasherLookAndFeel::GatecrasherLookAndFeel()
{
    // Approximate mid-fascia grey (section 1's gradient sits between #C6CCD0 and #A0A7AB) - only
    // ever visible for a frame before GatecrasherPanelBackground paints over it.
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFFB0B7BB));

    // Knob drag-value popup (KnobFilmstripComponent's setPopupDisplayEnabled) and the shared
    // TooltipWindow - styled like an LED window so they read as part of the same instrument.
    setColour(juce::BubbleComponent::backgroundColourId, GatecrasherTheme::Colour::ledWindowBg);
    setColour(juce::BubbleComponent::outlineColourId, GatecrasherTheme::Colour::ledWindowBorder);
    setColour(juce::TooltipWindow::backgroundColourId, GatecrasherTheme::Colour::ledWindowBg);
    setColour(juce::TooltipWindow::outlineColourId, GatecrasherTheme::Colour::ledWindowBorder);
    setColour(juce::TooltipWindow::textColourId, GatecrasherTheme::Colour::ledText);
}
