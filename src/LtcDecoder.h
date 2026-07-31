#pragma once

#include <JuceHeader.h>

// Thread-safe timecode info shared between audio and GUI threads
struct TimecodeInfo
{
    bool valid = false;
    int hours = 0;
    int mins = 0;
    int secs = 0;
    int frames = 0;
    int fps = 0;
    bool dropFrame = false;
    juce::int64 lastFrameCount = 0;
};

// RAII wrapper around libltc's C API
class LtcDecoder
{
public:
    LtcDecoder(double sampleRate);
    ~LtcDecoder();

    // Called from audio thread: feed audio samples to the LTC decoder
    void processBlock(const float* samples, int numSamples);

    // Called from GUI thread: get the latest decoded timecode
    TimecodeInfo getTimecode() const;

    // Re-initialise decoder for a new sample rate
    void setSampleRate(double newSampleRate);

    // Get the current sample rate
    double getSampleRate() const { return currentSampleRate; }

private:
    double currentSampleRate = 48000.0;
    juce::CriticalSection lock;
    TimecodeInfo latestTimecode;

    struct LTCDecoder* decoder = nullptr;
    juce::int64 frameCounter = 0;
    juce::int64 sampleOffset = 0;
};
