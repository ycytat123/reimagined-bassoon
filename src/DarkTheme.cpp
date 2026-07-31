#include "DarkTheme.h"

DarkTheme::DarkTheme()
{
    setColours();
}

void DarkTheme::setColours()
{
    // Window background
    setColour(juce::ResizableWindow::backgroundColourId,
              juce::Colour(0xff1a1d23));

    // Standard controls
    setColour(juce::Label::textColourId,
              juce::Colour(0xffd4d4d4));

    // Plugin background
    setColour(juce::DocumentWindow::textColourId,
              juce::Colour(0xffe0e0e0));
}

void DarkTheme::drawCornerResizer(juce::Graphics& g, int /*w*/, int /*h*/,
                                   bool /*isMouseOver*/, bool /*isMouseDragging*/)
{
    // No resize handle — fixed window
}
