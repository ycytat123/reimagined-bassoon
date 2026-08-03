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

private:
    LtcReaderAudioProcessor& processor;
    TimecodeDisplay timecodeDisplay;
    DarkTheme darkTheme;
    juce::Label machineIdLabel;   // shows the machine ID
    juce::Label licenseLabel;     // shows license status

    void updateLicenseLabel();

    // Signal-watchdog
    juce::int64 lastSeenFrameCount = 0;
    juce::uint32 lastSeenTime = 0;
    static constexpr juce::uint32 signalTimeoutMs = 250;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LtcReaderAudioProcessorEditor)
};