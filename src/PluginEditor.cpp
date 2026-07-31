#include "PluginEditor.h"

LtcReaderAudioProcessorEditor::LtcReaderAudioProcessorEditor(LtcReaderAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
{
    // Fixed window: 420 x 180
    setSize(420, 180);
    setResizable(false, false);

    // Apply dark theme
    setLookAndFeel(&darkTheme);

    addAndMakeVisible(timecodeDisplay);

    // Start the GUI update timer at ~33 Hz (30ms)
    startTimerHz(30);

    lastSeenTime = juce::Time::getMillisecondCounter();
}

LtcReaderAudioProcessorEditor::~LtcReaderAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void LtcReaderAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Solid dark background
    g.fillAll(juce::Colour(0xff1a1d23));
}

void LtcReaderAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    // 8px margin on each side
    timecodeDisplay.setBounds(area.reduced(8));
}

void LtcReaderAudioProcessorEditor::timerCallback()
{
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
