#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ids
{
    constexpr auto mode      = "mode";
    constexpr auto threshold = "threshold";
    constexpr auto ratio     = "ratio";
    constexpr auto attack    = "attack";
    constexpr auto release   = "release";
    constexpr auto makeup    = "makeup";
    constexpr auto depth     = "depth";
    constexpr auto detector  = "detector";
    constexpr auto lookahead = "lookahead";
}

K4SideChainProcessor::K4SideChainProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",      juce::AudioChannelSet::stereo(), true)
        .withInput  ("Sidechain",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output",     juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    visBufferDry.assign (visFifoSize, 0.0f);
    visBufferWet.assign (visFifoSize, 0.0f);

    // Latency only changes when lookahead changes; handle it off the audio thread.
    apvts.addParameterListener (ids::lookahead, this);
}

K4SideChainProcessor::~K4SideChainProcessor()
{
    apvts.removeParameterListener (ids::lookahead, this);
}

void K4SideChainProcessor::parameterChanged (const juce::String& paramID, float newValue)
{
    if (paramID == ids::lookahead)
        updateLatencyFromLookahead (newValue);
}

void K4SideChainProcessor::updateLatencyFromLookahead (float lookaheadMs)
{
    const double sr = getSampleRate();
    if (sr <= 0.0)
        return;

    const int maxLook = static_cast<int> (0.010 * sr) + 1;
    const int latency = juce::jlimit (0, maxLook,
        static_cast<int> (lookaheadMs * 0.001f * static_cast<float> (sr)));

    if (latency != getLatencySamples())
        setLatencySamples (latency);
}

void K4SideChainProcessor::pushVisualizerSamples (const float* dry, const float* wet, int n)
{
    const auto scope = visFifo.write (n);

    if (scope.blockSize1 > 0)
    {
        juce::FloatVectorOperations::copy (visBufferDry.data() + scope.startIndex1,
                                           dry, scope.blockSize1);
        juce::FloatVectorOperations::copy (visBufferWet.data() + scope.startIndex1,
                                           wet, scope.blockSize1);
    }
    if (scope.blockSize2 > 0)
    {
        juce::FloatVectorOperations::copy (visBufferDry.data() + scope.startIndex2,
                                           dry + scope.blockSize1, scope.blockSize2);
        juce::FloatVectorOperations::copy (visBufferWet.data() + scope.startIndex2,
                                           wet + scope.blockSize1, scope.blockSize2);
    }
}

int K4SideChainProcessor::readVisualizerSamples (float* dryDest, float* wetDest, int maxSamples)
{
    const int ready = juce::jmin (maxSamples, visFifo.getNumReady());
    const auto scope = visFifo.read (ready);

    if (scope.blockSize1 > 0)
    {
        juce::FloatVectorOperations::copy (dryDest,
                                           visBufferDry.data() + scope.startIndex1,
                                           scope.blockSize1);
        juce::FloatVectorOperations::copy (wetDest,
                                           visBufferWet.data() + scope.startIndex1,
                                           scope.blockSize1);
    }
    if (scope.blockSize2 > 0)
    {
        juce::FloatVectorOperations::copy (dryDest + scope.blockSize1,
                                           visBufferDry.data() + scope.startIndex2,
                                           scope.blockSize2);
        juce::FloatVectorOperations::copy (wetDest + scope.blockSize1,
                                           visBufferWet.data() + scope.startIndex2,
                                           scope.blockSize2);
    }
    return ready;
}

juce::AudioProcessorValueTreeState::ParameterLayout K4SideChainProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ids::mode, 1 }, "Mode",
        StringArray { "Compressor", "Gate", "Envelope Follower" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::threshold, 1 }, "Threshold",
        NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::ratio, 1 }, "Ratio",
        NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.5f), 4.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::attack, 1 }, "Attack",
        NormalisableRange<float> (0.1f, 200.0f, 0.1f, 0.4f), 10.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::release, 1 }, "Release",
        NormalisableRange<float> (1.0f, 1000.0f, 0.1f, 0.4f), 100.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::makeup, 1 }, "Makeup",
        NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::depth, 1 }, "Depth",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ids::detector, 1 }, "Detector",
        StringArray { "Peak", "RMS" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::lookahead, 1 }, "Lookahead",
        NormalisableRange<float> (0.0f, 10.0f, 0.1f), 0.0f));

    return layout;
}

bool K4SideChainProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Main in/out must match and be mono or stereo.
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainOut != mainIn)
        return false;

    // Sidechain (second input) may be disabled, mono, or stereo.
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled()
            && sc != juce::AudioChannelSet::mono()
            && sc != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void K4SideChainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock,
                    getMainBusNumInputChannels());
    const auto sz = static_cast<size_t> (juce::jmax (1, samplesPerBlock));
    monoDry.assign (sz, 0.0f);
    monoWet.assign (sz, 0.0f);

    updateLatencyFromLookahead (apvts.getRawParameterValue (ids::lookahead)->load());
}

void K4SideChainProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto mainBus = getBusBuffer (buffer, true, 0);
    auto scBus   = getBusBuffer (buffer, true, 1);

    // Gather current parameter values into the engine.
    SidechainEngine::Parameters p;
    p.mode = static_cast<SidechainEngine::Mode> (
        apvts.getRawParameterValue (ids::mode)->load());
    p.thresholdDb = apvts.getRawParameterValue (ids::threshold)->load();
    p.ratio       = apvts.getRawParameterValue (ids::ratio)->load();
    p.attackMs    = apvts.getRawParameterValue (ids::attack)->load();
    p.releaseMs   = apvts.getRawParameterValue (ids::release)->load();
    p.makeupDb    = apvts.getRawParameterValue (ids::makeup)->load();
    p.depth       = apvts.getRawParameterValue (ids::depth)->load();
    p.detector    = static_cast<SidechainEngine::Detector> (
        apvts.getRawParameterValue (ids::detector)->load());
    p.lookaheadMs = apvts.getRawParameterValue (ids::lookahead)->load();
    engine.setParameters (p);

    const int n      = mainBus.getNumSamples();
    const int mainCh = mainBus.getNumChannels();
    const bool feedViz = (n > 0 && n <= static_cast<int> (monoDry.size()));

    const auto mixToMono = [mainCh] (const juce::AudioBuffer<float>& buf,
                                     float* dst, int num)
    {
        juce::FloatVectorOperations::copy (dst, buf.getReadPointer (0), num);
        for (int ch = 1; ch < mainCh; ++ch)
            juce::FloatVectorOperations::add (dst, buf.getReadPointer (ch), num);
        if (mainCh > 1)
            juce::FloatVectorOperations::multiply (
                dst, 1.0f / static_cast<float> (mainCh), num);
    };

    // Capture the dry (pre-processing) signal for the visualizer.
    if (feedViz)
        mixToMono (mainBus, monoDry.data(), n);

    // If the host hasn't connected a sidechain, fall back to the main signal
    // as its own trigger so the plugin still does something sensible.
    if (scBus.getNumChannels() == 0)
        engine.process (mainBus, mainBus);
    else
        engine.process (mainBus, scBus);

    // (Latency is updated off the audio thread via the lookahead param listener.)

    // Capture the wet (post-processing) signal and push the dry/wet pair.
    if (feedViz)
    {
        mixToMono (mainBus, monoWet.data(), n);

        // Keep the FIFO from overflowing if the GUI isn't draining it.
        if (visFifo.getFreeSpace() < n)
            visFifo.read (n - visFifo.getFreeSpace());

        pushVisualizerSamples (monoDry.data(), monoWet.data(), n);
    }
}

juce::AudioProcessorEditor* K4SideChainProcessor::createEditor()
{
    return new K4SideChainEditor (*this);
}

void K4SideChainProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void K4SideChainProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new K4SideChainProcessor();
}
