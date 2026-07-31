#pragma once

#include <JuceHeader.h>
#include "LtcDecoder.h"

// Custom component that renders the timecode digits
class TimecodeDisplay : public juce::Component
{
public:
    TimecodeDisplay();

    void paint(juce::Graphics& g) override;

    // Update the displayed timecode (called from GUI timer)
    void setTimecode(const TimecodeInfo& tc);

    // Set whether an LTC signal is currently being received
    void setSignalPresent(bool present);

private:
    TimecodeInfo currentTimecode;
    bool signalPresent = false;

    // Colours for the display
    juce::Colour bgColour      { 0xff21252b };
    juce::Colour digitColour    { 0xfff0f0f0 };
    juce::Colour separatorColour{ 0xffe8a840 }; // amber
    juce::Colour dimmedColour  { 0xff555555 };
    juce::Colour noSignalText  { 0xff8b949e };
    juce::Colour signalGood     { 0xff4ec9b0 };
    juce::Colour signalLost     { 0xff555555 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimecodeDisplay)
};
