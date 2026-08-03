#include "PluginEditor.h"

LtcReaderAudioProcessorEditor::LtcReaderAudioProcessorEditor(LtcReaderAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(480, 200);
    setResizable(false, false);
    setLookAndFeel(&darkTheme);
    addAndMakeVisible(timecodeDisplay);

    addAndMakeVisible(machineIdLabel);
    machineIdLabel.setText("ID: " + processor.getMachineId(), juce::dontSendNotification);
    machineIdLabel.setFont(juce::FontOptions("Consolas", 11.0f, juce::Font::plain));
    machineIdLabel.setColour(juce::Label::textColourId, juce::Colour(0xff6b737c));
    machineIdLabel.setJustificationType(juce::Justification::left);

    addAndMakeVisible(licenseLabel);
    licenseLabel.setFont(juce::FontOptions("Segoe UI", 11.0f, juce::Font::plain));
    licenseLabel.setJustificationType(juce::Justification::centredRight);
    updateLicenseLabel();

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
        licenseLabel.setText("Licensed: " + processor.getLicenseStatus(),
                             juce::dontSendNotification);
        licenseLabel.setColour(juce::Label::textColourId,
                               juce::Colour(0xff4ec9b0));  // green
    }
    else
    {
        licenseLabel.setText("UNLICENSED", juce::dontSendNotification);
        licenseLabel.setColour(juce::Label::textColourId,
                               juce::Colour(0xfff44747));  // red
    }
}

void LtcReaderAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1d23));
}

void LtcReaderAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Bottom bar 24px: machine ID (left) | license status (right)
    auto bb = area.removeFromBottom(24).reduced(12, 0);
    machineIdLabel.setBounds(bb.removeFromLeft(220));
    licenseLabel.setBounds(bb);

    // Timecode display fills the rest
    timecodeDisplay.setBounds(area.reduced(8));
}

void LtcReaderAudioProcessorEditor::timerCallback()
{
    // If unlicensed, show dim "No Signal" — output already muted in processBlock
    if (!processor.isLicensed())
    {
        timecodeDisplay.setSignalPresent(false);
        timecodeDisplay.setTimecode(TimecodeInfo());
        return;
    }

    TimecodeInfo tc = processor.getLatestTimecode();
    juce::uint32 now = juce::Time::getMillisecondCounter();

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
