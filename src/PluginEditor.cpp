#include "PluginEditor.h"

LtcReaderAudioProcessorEditor::LtcReaderAudioProcessorEditor(LtcReaderAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
{
    // Fixed window: 420 x 200 (extra 20px for license status bar)
    setSize(420, 200);
    setResizable(false, false);

    // Apply dark theme
    setLookAndFeel(&darkTheme);

    addAndMakeVisible(timecodeDisplay);

    // License status label
    addAndMakeVisible(licenseLabel);
    licenseLabel.setJustificationType(juce::Justification::centredRight);
    licenseLabel.setFont(juce::FontOptions("Segoe UI", 11.0f, juce::Font::plain));

    updateLicenseLabel();

    // Start the GUI update timer at ~33 Hz (30ms)
    startTimerHz(30);

    lastSeenTime = juce::Time::getMillisecondCounter();
}

LtcReaderAudioProcessorEditor::~LtcReaderAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void LtcReaderAudioProcessorEditor::updateLicenseLabel()
{
    if (processor.isLicensed())
    {
        licenseLabel.setText(processor.getLicenseStatus(),
                             juce::dontSendNotification);
        licenseLabel.setColour(juce::Label::textColourId,
                               juce::Colour(0xff4ec9b0));  // green
    }
    else
    {
        licenseLabel.setText("[UNLICENSED] " + processor.getLicenseStatus(),
                             juce::dontSendNotification);
        licenseLabel.setColour(juce::Label::textColourId,
                               juce::Colour(0xfff44747));  // red
    }
}

void LtcReaderAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Solid dark background
    g.fillAll(juce::Colour(0xff1a1d23));
}

void LtcReaderAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    // License bar at the bottom (24px)
    licenseLabel.setBounds(area.removeFromBottom(24).reduced(12, 0));
    // Timecode display fills the rest
    timecodeDisplay.setBounds(area.reduced(8));
}

void LtcReaderAudioProcessorEditor::timerCallback()
{
    // Only show timecode if licensed
    if (!processor.isLicensed())
    {
        timecodeDisplay.setSignalPresent(false);
        timecodeDisplay.setTimecode(TimecodeInfo());
        return;
    }

    TimecodeInfo tc = processor.getLatestTimecode();

    juce::uint32 now = juce::Time::getMillisecondCounter();

    // Check if we've received new frames since last poll
    if (tc.lastFrameCount != lastSeenFrameCount)
    {
        lastSeenFrameCount = tc.lastFrameCount;
        lastSeenTime = now;
    }

    bool signalPresent = false;
    if (tc.lastFrameCount > 0)
    {
        juce::uint32 elapsed = now - lastSeenTime;
        signalPresent = (elapsed < signalTimeoutMs);
    }

    timecodeDisplay.setSignalPresent(signalPresent);
    if (signalPresent && tc.valid)
        timecodeDisplay.setTimecode(tc);
    else if (!signalPresent)
        timecodeDisplay.setTimecode(TimecodeInfo());
}
