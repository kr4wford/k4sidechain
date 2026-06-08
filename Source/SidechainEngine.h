#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

/**
    All three sidechain behaviours live here so the processor stays thin.

    The engine reads a control level from the sidechain signal (peak or RMS,
    optionally delayed by a lookahead), smooths it by attack/release, and
    applies a per-sample gain to the main signal according to the selected mode.
*/
class SidechainEngine
{
public:
    enum class Mode
    {
        Compressor = 0,   // duck main proportionally above threshold
        Gate,             // open main only while sidechain exceeds threshold
        EnvelopeFollower  // modulate main level by sidechain envelope * depth
    };

    enum class Detector
    {
        Peak = 0,
        RMS
    };

    struct Parameters
    {
        Mode     mode        = Mode::Compressor;
        Detector detector    = Detector::Peak;
        float    thresholdDb = -20.0f;   // Compressor + Gate + EnvelopeFollower
        float    ratio       = 4.0f;     // Compressor
        float    attackMs    = 10.0f;    // All
        float    releaseMs   = 100.0f;   // All
        float    makeupDb    = 0.0f;     // Compressor
        float    depth       = 1.0f;     // EnvelopeFollower (0..1)
        float    lookaheadMs = 0.0f;     // All (delays main to match detector)
    };

    void prepare (double sampleRate, int maxBlockSize, int numMainChannels);
    void reset();

    void setParameters (const Parameters& newParams) { params = newParams; }

    /** Reported latency in samples (from lookahead), for the host. */
    int getLatencySamples() const noexcept { return lookaheadSamples; }

    /** Processes the main block in place using the sidechain block as trigger.
        Both buffers must have the same number of samples. If the sidechain has
        fewer channels than main, channels are summed. */
    void process (juce::AudioBuffer<float>& mainBuffer,
                  const juce::AudioBuffer<float>& sidechainBuffer);

    /** Latest computed gain reduction in dB (for metering). */
    float getGainReductionDb() const noexcept { return currentGrDb; }

private:
    float computeTargetGain (float controlDb) const;

    double sampleRate     = 44100.0;
    int    maxBlock       = 512;
    int    numChannels    = 2;
    int    lookaheadSamples = 0;

    float  rmsAccum     = 0.0f;   // running mean-square for RMS detector
    float  smoothedGain = 1.0f;   // attack/release-smoothed output gain
    float  currentGrDb  = 0.0f;

    // Click-free makeup gain (ramped across each block).
    juce::SmoothedValue<float> makeupSmoother { 1.0f };

    // Delay line for lookahead on the main signal.
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;

    Parameters params;
};
