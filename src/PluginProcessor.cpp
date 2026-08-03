#include "PluginProcessor.h"
#include "PluginEditor.h"

LtcReaderAudioProcessor::LtcReaderAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::mono(), true)
        .withOutput("Output", juce::AudioChannelSet::mono(), true))
{
    performLicenseCheck();
}

LtcReaderAudioProcessor::~LtcReaderAudioProcessor() = default;

void LtcReaderAudioProcessor::performLicenseCheck()
{
    auto result = LicenseVerifier::checkStandardLocations();

    licensed = result.authorized;

    if (licensed)
    {
        licenseStatus = "Licensed to " + result.licensee;
        if (result.expiryDate != "perpetual")
            licenseStatus += " (expires " + result.expiryDate + ")";
    }
    else
    {
        licenseStatus = result.error;
    }
}

bool LtcReaderAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support mono and stereo, input channels must match output channels
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainInputChannels() > 0
        && layouts.getMainInputChannels() <= 2;
}

void LtcReaderAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    ltcDecoder = std::make_unique<LtcDecoder>(sampleRate);
}

void LtcReaderAudioProcessor::releaseResources()
{
    // Keep decoder alive
}

void LtcReaderAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Mute if NOT licensed: clear ALL output channels (full silence)
    if (!licensed)
    {
        for (auto ch = 0; ch < totalNumOutputChannels; ++ch)
            buffer.clear(ch, 0, buffer.getNumSamples());
        return;
    }

    // --- Licensed: normal LTC decoding ---


    // Pass-through: copy input to output
    for (auto ch = 0; ch < totalNumInputChannels; ++ch)
    {
        if (ch < totalNumOutputChannels && buffer.getReadPointer(ch) != buffer.getWritePointer(ch))
            buffer.copyFrom(ch, 0, buffer.getReadPointer(ch), buffer.getNumSamples());
    }

    // Decode LTC from channel 0
    if (ltcDecoder && totalNumInputChannels > 0)
    {
        ltcDecoder->processBlock(buffer.getReadPointer(0), buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* LtcReaderAudioProcessor::createEditor()
{
    return new LtcReaderAudioProcessorEditor(*this);
}

TimecodeInfo LtcReaderAudioProcessor::getLatestTimecode() const
{
    if (ltcDecoder)
        return ltcDecoder->getTimecode();
    return TimecodeInfo();
}

// ==============================================================================
// Plugin factory function — required by JUCE and all plugin formats
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LtcReaderAudioProcessor();
}
