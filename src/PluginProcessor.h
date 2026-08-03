#pragma once

#include <JuceHeader.h>
#include "LtcDecoder.h"
#include "LicenseVerifier.h"

class LtcReaderAudioProcessor : public juce::AudioProcessor
{
public:
    LtcReaderAudioProcessor();
    ~LtcReaderAudioProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LTC Reader"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Thread-safe access for the GUI
    TimecodeInfo getLatestTimecode() const;

    // License state (checked once at construction)
    bool isLicensed() const { return licensed; }
    juce::String getLicenseStatus() const { return licenseStatus; }
    juce::String getMachineId() const { return LicenseVerifier::getMachineId(); }

private:
    std::unique_ptr<LtcDecoder> ltcDecoder;
    bool muted = true;
    bool licensed = false;
    juce::String licenseStatus;

    void performLicenseCheck();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LtcReaderAudioProcessor)
};
