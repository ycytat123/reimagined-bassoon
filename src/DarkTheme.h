#pragma once

#include <JuceHeader.h>

// Dark theme LookAndFeel matching the existing project aesthetic
class DarkTheme : public juce::LookAndFeel_V4
{
public:
    DarkTheme();

    void drawCornerResizer(juce::Graphics& g, int w, int h,
                           bool isMouseOver, bool isMouseDragging) override;

private:
    void setColours();
};
