#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "WaveformScope.h"

class K4SideChainEditor : public juce::AudioProcessorEditor
{
public:
    explicit K4SideChainEditor (K4SideChainProcessor&);
    ~K4SideChainEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateVisibleControls();

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BoxAttachment    = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct LabeledKnob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setupKnob (LabeledKnob&, const juce::String& paramID, const juce::String& text);

    K4SideChainProcessor& processor;

    WaveformScope  visualizer;
    juce::Label    titleLabel;

    juce::ComboBox modeBox;
    juce::Label    modeLabel;
    std::unique_ptr<BoxAttachment> modeAttachment;

    juce::ComboBox detectorBox;
    juce::Label    detectorLabel;
    std::unique_ptr<BoxAttachment> detectorAttachment;

    LabeledKnob threshold, ratio, attack, release, makeup, depth, lookahead;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K4SideChainEditor)
};
