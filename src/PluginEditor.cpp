#include "PluginEditor.h"

LtcReaderAudioProcessorEditor::LtcReaderAudioProcessorEditor(LtcReaderAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(480, 220);
    setResizable(false, false);
    setLookAndFeel(&darkTheme);

    addAndMakeVisible(timecodeDisplay);
    timecodeDisplay.setLicensed(processor.isLicensed());

    // Machine-ID label — only shown when unlicensed, clickable to copy
    addAndMakeVisible(machineIdLabel);
    machineIdLabel.setFont(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::bold));
    machineIdLabel.setJustificationType(juce::Justification::left);
    machineIdLabel.addMouseListener(this, false);
    machineIdLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    // License status label
    addAndMakeVisible(licenseLabel);
    licenseLabel.setFont(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
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
        // Activated: hide machine ID entirely
        machineIdLabel.setVisible(false);

        licenseLabel.setText("ACTIVE", juce::dontSendNotification);
        licenseLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4af0c0));
    }
    else
    {
        // Unlicensed: show machine ID so the user can send it to the vendor
        machineIdLabel.setVisible(true);
        machineIdLabel.setText("MACHINE ID  " + processor.getMachineId() +
                               "   [CLICK TO COPY]", juce::dontSendNotification);
        machineIdLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffb347));

        licenseLabel.setText("UNLICENSED", juce::dontSendNotification);
        licenseLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff5f56));
    }
}

void LtcReaderAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background gradient, matching the HUD
    auto b = getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour(0xff0d1420), 0.0f, 0.0f,
                            juce::Colour(0xff05080e), 0.0f, b.getHeight(), false);
    g.setGradientFill(bg);
    g.fillRect(b);
}

void LtcReaderAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Bottom status bar
    auto bb = area.removeFromBottom(30).reduced(14, 0);
    machineIdLabel.setBounds(bb.removeFromLeft(area.getWidth() * 0.62f));
    licenseLabel.setBounds(bb);

    // HUD display fills the rest
    timecodeDisplay.setBounds(area.reduced(6, 4));
}

void LtcReaderAudioProcessorEditor::timerCallback()
{
    // Keep license state fresh
    timecodeDisplay.setLicensed(processor.isLicensed());

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

void LtcReaderAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    if (e.eventComponent == &machineIdLabel)
    {
        juce::SystemClipboard::copyTextToClipboard(processor.getMachineId());
        // Brief feedback flash
        machineIdLabel.setText("MACHINE ID  " + processor.getMachineId() +
                               "   [COPIED]", juce::dontSendNotification);
        machineIdLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4af0c0));

        startTimerHz(30); // keep timer running; revert text after a short delay
        machineIdLabel.setAlpha(0.6f);

        // Revert label after 1.5s
        juce::Timer::callAfterDelay(1500, [this]()
        {
            machineIdLabel.setAlpha(1.0f);
            if (machineIdLabel.isVisible())
                updateLicenseLabel();
        });
    }
}
