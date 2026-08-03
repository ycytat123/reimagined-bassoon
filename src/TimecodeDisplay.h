#pragma once

#include <JuceHeader.h>
#include "LtcDecoder.h"

// ==============================================================================
// Sci-fi HUD timecode display: gradient bg, grid, scan line, glowing digits,
// LED status, title header.
// ==============================================================================
class TimecodeDisplay : public juce::Component
{
public:
    TimecodeDisplay();

    void paint(juce::Graphics& g) override;

    // Update the displayed timecode (called from GUI timer)
    void setTimecode(const TimecodeInfo& tc);

    // Set whether an LTC signal is currently being received
    void setSignalPresent(bool present);

    // Set whether the plugin is licensed (affects display mode)
    void setLicensed(bool lic);

private:
    TimecodeInfo currentTimecode;
    bool signalPresent = false;
    bool licensed = true;

    // ── Palette (sci-fi HUD) ──
    juce::Colour bgTop      { 0xff141c29 };
    juce::Colour bgBottom   { 0xff090e17 };
    juce::Colour panelBorder{ 0xff26364c };
    juce::Colour accent     { 0xff00e5ff };  // cyan
    juce::Colour textMain   { 0xffe3f9ff };
    juce::Colour textDim    { 0xff5b6d80 };
    juce::Colour amber      { 0xffffb347 };
    juce::Colour green      { 0xff4af0c0 };
    juce::Colour red        { 0xffff5f56 };
    juce::Colour grey       { 0xff3f4c5c };

    // ── Helpers ──
    void drawGrid(juce::Graphics& g, juce::Rectangle<float> b);
    void drawScanLine(juce::Graphics& g, float height);
    void drawLed(juce::Graphics& g, float x, float y);
    void drawHeader(juce::Graphics& g, juce::Rectangle<float> b);
    void drawGlowText(juce::Graphics& g, const juce::String& text,
                      const juce::Font& font, float x, float y,
                      const juce::Colour& glowColour, float blurRadius);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimecodeDisplay)
};
