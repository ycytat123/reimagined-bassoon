#include "TimecodeDisplay.h"

TimecodeDisplay::TimecodeDisplay()
{
}

// ==============================================================================
// paint
// ==============================================================================
void TimecodeDisplay::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float w = b.getWidth();
    const float h = b.getHeight();

    // ── Background: vertical gradient + vignette ──
    juce::ColourGradient bg(bgTop, 0.0f, 0.0f, bgBottom, 0.0f, h, false);
    g.setGradientFill(bg);
    g.fillRect(b);

    juce::ColourGradient vignette(juce::Colour(0x00000000), w * 0.5f, h * 0.45f,
                                  juce::Colour(0x77000000), w * 1.1f, h * 1.15f, true);
    g.setGradientFill(vignette);
    g.fillRect(b);

    drawGrid(g, b);

    // ── Panel border + top accent ──
    auto panel = b.reduced(4.0f);
    g.setColour(panelBorder);
    g.drawRoundedRectangle(panel, 8.0f, 1.2f);

    g.setColour(accent);
    g.fillRect(8.0f, 5.5f, w - 16.0f, 2.0f);

    // ── Title header + LED ──
    drawHeader(g, b);
    drawLed(g, w - 26.0f, 20.0f);

    // ── Scan line overlay ──
    drawScanLine(g, h);

    // ── Timecode / status text ──
    juce::String timeStr;
    juce::Colour strColour = textMain;
    juce::Colour glowColour = accent;
    const char sep = currentTimecode.dropFrame ? ';' : ':';

    if (!licensed)
    {
        // Unlicensed: big red "NO LICENSE", hint below
        auto font = juce::Font(juce::FontOptions(
            juce::Font::getDefaultMonospacedFontName(), 34.0f, juce::Font::bold));
        juce::String msg = "NO LICENSE";
        float tw = juce::GlyphArrangement::getStringWidth(font, msg);
        drawGlowText(g, msg, font, (w - tw) * 0.5f,
                     h * 0.42f + font.getHeight() * 0.75f, red, 10.0f);

        auto hintFont = juce::Font(juce::FontOptions(
            juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
        juce::String hint = "SEND MACHINE ID BELOW TO ACTIVATE";
        float hw = juce::GlyphArrangement::getStringWidth(hintFont, hint);
        g.setColour(amber);
        g.setFont(hintFont);
        g.drawSingleLineText(hint, juce::roundToInt((w - hw) * 0.5f),
                             juce::roundToInt(h * 0.55f));
        return;
    }

    if (!signalPresent)
    {
        timeStr = "00:00:00:00";
        strColour = textDim;
        glowColour = juce::Colour(0x66263c52);
    }
    else if (currentTimecode.valid)
    {
        timeStr = juce::String::formatted(
            "%02d%c%02d%c%02d%c%02d",
            currentTimecode.hours,  sep,
            currentTimecode.mins,   sep,
            currentTimecode.secs,   sep,
            currentTimecode.frames);
        strColour = textMain;
        glowColour = signalPresent ? accent : juce::Colour(0x6600e5ff);
    }
    else
    {
        timeStr = "00:00:00:00";
        strColour = textDim;
        glowColour = juce::Colour(0x66263c52);
    }

    // Font size proportional to panel
    auto textArea = b.reduced(20.0f, 44.0f);
    const float fontSize = juce::jmin(textArea.getWidth() * 0.085f,
                                      textArea.getHeight() * 0.62f, 78.0f);
    juce::Font timeFont(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(), fontSize, juce::Font::bold));

    float textW = juce::GlyphArrangement::getStringWidth(timeFont, timeStr);
    float textH = timeFont.getHeight();
    float x = (w - textW) * 0.5f;
    float y = (h - textH) * 0.5f + textH * 0.72f;

    drawGlowText(g, timeStr, timeFont, x, y, glowColour, signalPresent ? 12.0f : 6.0f);

    // ── Bottom-right: frame rate / signal info ──
    juce::Font infoFont(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    juce::String infoText;

    if (signalPresent && currentTimecode.valid)
    {
        g.setColour(amber);
        if (currentTimecode.fps == 30 && currentTimecode.dropFrame)
            infoText = "29.97 DF";
        else if (currentTimecode.fps == 30)
            infoText = "30";
        else
            infoText = juce::String(currentTimecode.fps);
        infoText += " FPS";
    }
    else
    {
        g.setColour(textDim);
        infoText = "NO SIGNAL";
    }

    float infoW = juce::GlyphArrangement::getStringWidth(infoFont, infoText);
    g.setFont(infoFont);
    g.drawSingleLineText(infoText, juce::roundToInt(w - infoW - 18.0f),
                         juce::roundToInt(h - 16.0f));

    // Bottom accent separator
    g.setColour(accent.withAlpha(0.35f));
    g.fillRect(8.0f, h - 4.5f, w - 16.0f, 1.5f);
}

// ==============================================================================
// Draw helpers
// ==============================================================================
void TimecodeDisplay::drawGrid(juce::Graphics& g, juce::Rectangle<float> b)
{
    g.setColour(juce::Colour(0x08ffffff));
    const float gridSize = 34.0f;
    for (float x = b.getX(); x < b.getRight(); x += gridSize)
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());
    for (float y = b.getY(); y < b.getBottom(); y += gridSize)
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());
}

void TimecodeDisplay::drawScanLine(juce::Graphics& g, float height)
{
    auto now = juce::Time::getMillisecondCounter() % 4200u;
    float t = static_cast<float>(now) / 4200.0f;
    float y = t * (height + 90.0f) - 45.0f;

    juce::ColourGradient line(juce::Colour(0x16ffffff), 0.0f, y,
                              juce::Colour(0x00ffffff), 0.0f, y + 42.0f, false);
    g.setGradientFill(line);
    g.fillRect(0.0f, y, static_cast<float>(getWidth()), 2.0f);
}

void TimecodeDisplay::drawLed(juce::Graphics& g, float x, float y)
{
    juce::Colour col;
    if (!licensed)        col = red;
    else if (signalPresent) col = green;
    else                  col = grey;

    g.setColour(col.withAlpha(0.28f));
    g.fillEllipse(x - 11.0f, y - 11.0f, 22.0f, 22.0f);
    g.setColour(col);
    g.fillEllipse(x - 4.5f, y - 4.5f, 9.0f, 9.0f);
}

void TimecodeDisplay::drawHeader(juce::Graphics& g, juce::Rectangle<float> b)
{
    auto font = juce::Font(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
    g.setColour(textDim);
    g.setFont(font);
    g.drawText("LTC READER", b.reduced(18.0f, 10.0f).removeFromTop(20.0f),
               juce::Justification::topLeft);

    // Right-side: drop-frame marker or sample rate
    auto rightFont = juce::Font(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
    g.setColour(accent.withAlpha(0.8f));
    g.setFont(rightFont);
    g.drawText("SMPTE 80-BIT", b.reduced(18.0f, 10.0f).removeFromTop(20.0f),
               juce::Justification::topRight);
}

// ==============================================================================
// Glowing text: Gaussian-blur the layer, draw the halo, then crisp glyphs.
// ==============================================================================
void TimecodeDisplay::drawGlowText(juce::Graphics& g, const juce::String& text,
                                   const juce::Font& font, float x, float y,
                                   const juce::Colour& glowColour, float blurRadius)
{
    int iw = getWidth();
    int ih = getHeight();

    // Render text to an offscreen layer
    juce::Image layer(juce::Image::ARGB, iw, ih, true);
    {
        juce::Graphics lg(layer);
        lg.setColour(glowColour);
        lg.setFont(font);
        lg.drawSingleLineText(text, juce::roundToInt(x), juce::roundToInt(y));
    }

    // Gaussian blur for the glow halo (kernel sized just larger than radius*2)
    int kSize = juce::jmax(3, juce::roundToInt(blurRadius) * 2 + 1);
    juce::ImageConvolutionKernel kernel(kSize);
    kernel.createGaussianBlur(blurRadius);

    juce::Image glow(juce::Image::ARGB, iw, ih, true);
    kernel.applyToImage(glow, layer, juce::Rectangle<int>(0, 0, iw, ih));

    // Draw the halo, then the crisp glyphs
    g.drawImageAt(glow, 0, 0);
    g.setColour(glowColour);
    g.setFont(font);
    g.drawSingleLineText(text, juce::roundToInt(x), juce::roundToInt(y));
}

// ==============================================================================
// Setters
// ==============================================================================
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

void TimecodeDisplay::setLicensed(bool lic)
{
    if (licensed != lic)
    {
        licensed = lic;
        repaint();
    }
}
