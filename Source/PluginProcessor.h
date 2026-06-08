#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SidechainEngine.h"

class K4SideChainProcessor : public juce::AudioProcessor,
                             private juce::AudioProcessorValueTreeState::Listener
{
public:
    K4SideChainProcessor();
    ~K4SideChainProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "k4 SideChain"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    SidechainEngine engine;

    float getGainReductionDb() const noexcept { return engine.getGainReductionDb(); }

    /** Lock-free read of recent mono dry/wet samples for the visualizer.
        Both destinations receive the same number of samples (the return value). */
    int readVisualizerSamples (float* dryDest, float* wetDest, int maxSamples);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void parameterChanged (const juce::String& paramID, float newValue) override;
    void updateLatencyFromLookahead (float lookaheadMs);

    void pushVisualizerSamples (const float* dry, const float* wet, int n);

    static constexpr int visFifoSize = 1 << 15; // 32768
    juce::AbstractFifo  visFifo { visFifoSize };
    std::vector<float>  visBufferDry, visBufferWet;
    std::vector<float>  monoDry, monoWet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K4SideChainProcessor)
};
