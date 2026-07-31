#include "TimecodeDisplay.h"

TimecodeDisplay::TimecodeDisplay()
{
}

void TimecodeDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Background fill — rounded rect
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 8.0f);

    // Calculate font size proportional to component height
    const float fontSize = juce::jmin(h * 0.42f, w * 0.13f, 96.0f);
    juce::Font timeFont(juce::FontOptions("Consolas", fontSize, juce::Font::bold));

    juce::String timeStr;
    juce::Colour strColour = dimmedColour;

    if (!signalPresent)
    {
        timeStr = "00:00:00:00";
        strColour = dimmedColour;
    }
    else if (currentTimecode.valid)
    {
        const char sep = currentTimecode.dropFrame ? ';' : ':';
        timeStr = juce::String::formatted(
            "%02d%c%02d%c%02d%c%02d",
            currentTimecode.hours,  sep,
            currentTimecode.mins,   sep,
            currentTimecode.secs,   sep,
            currentTimecode.frames);
        strColour = digitColour;
    }
    else
    {
        timeStr = "00:00:00:00";
        strColour = dimmedColour;
    }

    // Draw the timecode centred in the component
    g.setColour(strColour);
    g.setFont(timeFont);

    // Measure text to centre it using GlyphArrangement::getStringWidth (static method)
    float textW = juce::GlyphArrangement::getStringWidth(timeFont, timeStr);
    float textH = timeFont.getHeight();
    float x = (w - textW) * 0.5f;
    float y = (h - textH) * 0.5f + textH * 0.75f;

    g.drawSingleLineText(timeStr, juce::roundToInt(x), juce::roundToInt(y));

    // Separator colons/semicolons overlay in amber
    for (int i = 1; i <= 3; ++i)
    {
        // Find the position of the separator (at positions 2, 5, 8)
        int sepPos = i * 3 - 1;
        juce::String prefix = timeStr.substring(0, sepPos);
        float prefixW = juce::GlyphArrangement::getStringWidth(timeFont, prefix);
        juce::String sepStr = timeStr.substring(sepPos, sepPos + 1);
        float sepW = juce::GlyphArrangement::getStringWidth(timeFont, sepStr);

        g.setColour(signalPresent ? separatorColour : dimmedColour);
        g.drawSingleLineText(
            juce::String::charToString(timeStr[sepPos]),
            juce::roundToInt(x + prefixW + sepW * 0.33f), juce::roundToInt(y));
    }

    // "No Signal" or frame rate indicator at the bottom-right
    juce::Font infoFont(juce::FontOptions("Segoe UI", fontSize * 0.22f, juce::Font::plain));
    g.setFont(infoFont);

    juce::String infoText;
    if (!signalPresent)
    {
        g.setColour(noSignalText);
        infoText = "No Signal";
    }
    else if (currentTimecode.valid)
    {
        g.setColour(separatorColour);
        juce::String fpsLabel;
        if (currentTimecode.fps == 30 && currentTimecode.dropFrame)
            fpsLabel = "29.97 DF";
        else if (currentTimecode.fps == 30)
            fpsLabel = "30";
        else
            fpsLabel = juce::String(currentTimecode.fps);

        infoText = fpsLabel + " fps";
    }

    if (infoText.isNotEmpty())
    {
        float infoW = juce::GlyphArrangement::getStringWidth(infoFont, infoText);
        g.drawSingleLineText(infoText,
            juce::roundToInt(w - infoW - 24.0f),
            juce::roundToInt(h - fontSize * 0.22f - 12.0f));
    }

    // Signal status dot (top-right corner)
    const float dotR = 6.0f;
    const float dotX = w - 24.0f;
    const float dotY = 16.0f;
    g.setColour(signalPresent ? signalGood : signalLost);
    g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
}

void TimecodeDisplay::setTimecode(const TimecodeInfo& tc)
{
    currentTimecode = tc;
    repaint();
}

void TimecodeDisplay::setSignalPresent(bool present)
{
    if (signalPresent != present)
    {
        signalPresent = present;
        repaint();
    }
}
