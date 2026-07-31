#include "LtcDecoder.h"

extern "C" {
#include "ltc.h"
}

LtcDecoder::LtcDecoder(double sampleRate)
    : currentSampleRate(sampleRate)
{
    // apv = samples per frame: e.g. 48000 / 30 = 1600
    int apv = static_cast<int>(currentSampleRate / 30.0);
    // queue_size = 32 frames of buffering
    decoder = ltc_decoder_create(apv, 32);
}

LtcDecoder::~LtcDecoder()
{
    if (decoder)
    {
        ltc_decoder_free(decoder);
        decoder = nullptr;
    }
}

void LtcDecoder::processBlock(const float* samples, int numSamples)
{
    if (!decoder || numSamples <= 0)
        return;

    // Feed float samples to libltc
    ltc_decoder_write_float(decoder, const_cast<float*>(samples),
                            static_cast<size_t>(numSamples), sampleOffset);
    sampleOffset += numSamples;

    // Read any completed frames from the decoder's queue
    LTCFrameExt frameExt;
    while (ltc_decoder_read(decoder, &frameExt) == 1)
    {
        SMPTETimecode stime;
        ltc_frame_to_time(&stime, &frameExt.ltc, 0);

        juce::ScopedLock sl(lock);
        latestTimecode.valid = true;
        latestTimecode.hours = stime.hours;
        latestTimecode.mins = stime.mins;
        latestTimecode.secs = stime.secs;
        latestTimecode.frames = stime.frame;
        latestTimecode.dropFrame = (frameExt.ltc.dfbit == 1);

        // Determine frame rate from the sample timing
        double samplesPerFrame = frameExt.off_end - frameExt.off_start + 1.0;
        if (samplesPerFrame > 0)
        {
            double detectedFps = currentSampleRate / samplesPerFrame;
            // Round to standard frame rates
            if (detectedFps >= 29.0 && detectedFps <= 30.5)
            {
                if (latestTimecode.dropFrame)
                    latestTimecode.fps = 30; // 29.97 DF, stored as 30 for display
                else
                    latestTimecode.fps = 30;
            }
            else if (detectedFps >= 24.0 && detectedFps <= 25.5)
            {
                latestTimecode.fps = 25;
            }
            else if (detectedFps >= 23.0 && detectedFps <= 24.5)
            {
                latestTimecode.fps = 24;
            }
            else
            {
                latestTimecode.fps = static_cast<int>(detectedFps + 0.5);
            }
        }

        latestTimecode.lastFrameCount = ++frameCounter;
    }
}

TimecodeInfo LtcDecoder::getTimecode() const
{
    juce::ScopedLock sl(lock);
    return latestTimecode;
}

void LtcDecoder::setSampleRate(double newSampleRate)
{
    if (newSampleRate == currentSampleRate)
        return;

    currentSampleRate = newSampleRate;

    // Flush the decoder queue and recreate
    if (decoder)
    {
        ltc_decoder_queue_flush(decoder);
    }

    sampleOffset = 0;
    frameCounter = 0;

    juce::ScopedLock sl(lock);
    latestTimecode = TimecodeInfo();
}
