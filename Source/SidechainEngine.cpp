#include "SidechainEngine.h"

void SidechainEngine::prepare (double newSampleRate, int maxBlockSize, int numMainChannels)
{
    sampleRate  = newSampleRate;
    maxBlock    = juce::jmax (1, maxBlockSize);
    numChannels = juce::jmax (1, numMainChannels);

    // Size the lookahead delay line for the largest lookahead we allow (10 ms).
    const int maxLookahead = static_cast<int> (0.010 * sampleRate) + 1;
    delayBuffer.setSize (numChannels, maxLookahead + maxBlock, false, true, true);

    // ~20 ms ramp removes zipper noise when makeup is dragged.
    makeupSmoother.reset (sampleRate, 0.02);
    reset();
}

void SidechainEngine::reset()
{
    rmsAccum      = 0.0f;
    smoothedGain  = 1.0f;
    currentGrDb   = 0.0f;
    delayWritePos = 0;
    delayBuffer.clear();
    makeupSmoother.setCurrentAndTargetValue (1.0f);
}

float SidechainEngine::computeTargetGain (float controlDb) const
{
    switch (params.mode)
    {
        case Mode::Compressor:
        {
            if (controlDb <= params.thresholdDb)
                return 1.0f;

            const float overshoot = controlDb - params.thresholdDb;
            const float grDb      = overshoot * (1.0f - 1.0f / params.ratio);
            return juce::Decibels::decibelsToGain (-grDb);
        }

        case Mode::Gate:
            return controlDb > params.thresholdDb ? 1.0f : 0.0f;

        case Mode::EnvelopeFollower:
        {
            // Only react once the trigger clears the threshold; below it the
            // signal passes untouched. Above it, scale ducking up to depth.
            if (controlDb <= params.thresholdDb)
                return 1.0f;

            // Normalise overshoot across a fixed 24 dB range to 0..1.
            const float range = juce::jlimit (0.0f, 1.0f,
                                              (controlDb - params.thresholdDb) / 24.0f);
            return 1.0f - params.depth * range;
        }
    }

    return 1.0f;
}

void SidechainEngine::process (juce::AudioBuffer<float>& mainBuffer,
                               const juce::AudioBuffer<float>& sidechainBuffer)
{
    const int numSamples = mainBuffer.getNumSamples();
    const int mainChans  = mainBuffer.getNumChannels();
    const int scChans    = sidechainBuffer.getNumChannels();

    lookaheadSamples = juce::jlimit (
        0,
        delayBuffer.getNumSamples() - maxBlock,
        static_cast<int> (params.lookaheadMs * 0.001f * static_cast<float> (sampleRate)));

    const auto coefFor = [this] (float ms)
    {
        const float t = juce::jmax (0.1f, ms) * 0.001f;
        return std::exp (-1.0f / (static_cast<float> (sampleRate) * t));
    };
    // Attack/release now smooth the GAIN itself, so even the gate's hard
    // on/off target is turned into a click-free fade.
    const float attCoef = coefFor (params.attackMs);
    const float relCoef = coefFor (params.releaseMs);

    // Fixed, modest RMS window so detection no longer fights attack/release.
    const float rmsCoef = std::exp (-1.0f / (static_cast<float> (sampleRate) * 0.010f));

    makeupSmoother.setTargetValue (juce::Decibels::decibelsToGain (params.makeupDb));
    const int delayLen = delayBuffer.getNumSamples();

    float maxGrDb = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        // --- Detector: collapse sidechain to a mono level (instantaneous) ---
        float scSample = 0.0f;
        for (int ch = 0; ch < scChans; ++ch)
            scSample += sidechainBuffer.getSample (ch, n);
        if (scChans > 0)
            scSample /= static_cast<float> (scChans);

        float level;
        if (params.detector == Detector::RMS)
        {
            rmsAccum = rmsCoef * rmsAccum + (1.0f - rmsCoef) * (scSample * scSample);
            level    = std::sqrt (rmsAccum);
        }
        else
        {
            level = std::abs (scSample);
        }

        const float controlDb  = juce::Decibels::gainToDecibels (level, -100.0f);
        const float targetGain = computeTargetGain (controlDb);

        // --- Attack/release smoothing of the gain (attack = ducking harder) ---
        const float coef = targetGain < smoothedGain ? attCoef : relCoef;
        smoothedGain = coef * smoothedGain + (1.0f - coef) * targetGain;

        const float makeup = makeupSmoother.getNextValue();

        maxGrDb = juce::jmax (maxGrDb,
                              -juce::Decibels::gainToDecibels (smoothedGain, -100.0f));

        // --- Apply gain to the (optionally delayed) main signal ---
        for (int ch = 0; ch < mainChans; ++ch)
        {
            float* main = mainBuffer.getWritePointer (ch);

            if (lookaheadSamples > 0 && ch < delayBuffer.getNumChannels())
            {
                float* dl = delayBuffer.getWritePointer (ch);
                const int writeIdx = (delayWritePos + n) % delayLen;
                const int readIdx   = (writeIdx - lookaheadSamples + delayLen) % delayLen;

                const float delayed = dl[readIdx];
                dl[writeIdx] = main[n];
                main[n] = delayed * smoothedGain * makeup;
            }
            else
            {
                main[n] *= smoothedGain * makeup;
            }
        }
    }

    if (lookaheadSamples > 0)
        delayWritePos = (delayWritePos + numSamples) % delayLen;

    currentGrDb = maxGrDb;
}
