#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TimecodeDisplay.h"
#include "DarkTheme.h"

class LtcReaderAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    explicit LtcReaderAudioProcessorEditor(LtcReaderAudioProcessor& p);
    ~LtcReaderAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Click the machine-ID label to copy it to the clipboard
    void mouseUp(const juce::MouseEvent& e) override;

private:
    LtcReaderAudioProcessor& processor;
    TimecodeDisplay timecodeDisplay;
    DarkTheme darkTheme;
    juce::Label machineIdLabel;   // only visible when unlicensed
    juce::Label licenseLabel;     // license status

    void updateLicenseLabel();

    // Signal-watchdog
    juce::int64 lastSeenFrameCount = 0;
    juce::uint32 lastSeenTime = 0;
    static constexpr juce::uint32 signalTimeoutMs = 250;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LtcReaderAudioProcessorEditor)
};
