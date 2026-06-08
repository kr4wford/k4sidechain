#include "PluginEditor.h"

namespace
{
    constexpr int width  = 640;
    constexpr int height = 380;
}

K4SideChainEditor::K4SideChainEditor (K4SideChainProcessor& p)
    : AudioProcessorEditor (p), processor (p), visualizer (p)
{
    // Visualizer added first so it sits behind every control.
    addAndMakeVisible (visualizer);

    titleLabel.setText ("k4 SideChain", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (modeLabel);

    modeBox.addItemList ({ "Compressor", "Gate", "Envelope Follower" }, 1);
    addAndMakeVisible (modeBox);
    modeAttachment = std::make_unique<BoxAttachment> (
        processor.apvts, "mode", modeBox);
    modeBox.onChange = [this] { updateVisibleControls(); };

    detectorLabel.setText ("Detector", juce::dontSendNotification);
    detectorLabel.setJustificationType (juce::Justification::centred);
    detectorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (detectorLabel);

    detectorBox.addItemList ({ "Peak", "RMS" }, 1);
    addAndMakeVisible (detectorBox);
    detectorAttachment = std::make_unique<BoxAttachment> (
        processor.apvts, "detector", detectorBox);

    setupKnob (threshold, "threshold", "Threshold");
    setupKnob (ratio,     "ratio",     "Ratio");
    setupKnob (attack,    "attack",    "Attack");
    setupKnob (release,   "release",   "Release");
    setupKnob (makeup,    "makeup",    "Makeup");
    setupKnob (depth,     "depth",     "Depth");
    setupKnob (lookahead, "lookahead", "Lookahead");

    setSize (width, height);
    updateVisibleControls();
}

K4SideChainEditor::~K4SideChainEditor() = default;

void K4SideChainEditor::setupKnob (LabeledKnob& k, const juce::String& paramID,
                                   const juce::String& text)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<SliderAttachment> (
        processor.apvts, paramID, k.slider);
}

void K4SideChainEditor::updateVisibleControls()
{
    const int mode = modeBox.getSelectedItemIndex(); // 0=Comp 1=Gate 2=Env

    const auto show = [] (LabeledKnob& k, bool v)
    {
        k.slider.setVisible (v);
        k.label.setVisible (v);
    };

    show (threshold, true);
    show (ratio,     mode == 0);
    show (makeup,    mode == 0);
    show (depth,     mode == 2);
    show (attack,    true);
    show (release,   true);
    show (lookahead, true);

    resized();
}

void K4SideChainEditor::paint (juce::Graphics& g)
{
    // Only visible where the GL view isn't (panel mode leaves a control strip).
    g.fillAll (juce::Colour (0xff121218));
}

void K4SideChainEditor::resized()
{
    // Visualizer is a full-bleed background behind everything.
    visualizer.setBounds (getLocalBounds());

    // Header strip.
    auto header = getLocalBounds().removeFromTop (40).reduced (12, 6);
    titleLabel.setBounds (header.removeFromLeft (200));

    // Controls sit over the lower portion of the background.
    auto area = getLocalBounds().removeFromBottom (height - 190).reduced (12);

    auto top = area.removeFromTop (46);
    modeLabel.setBounds (top.removeFromLeft (46));
    modeBox.setBounds (top.removeFromLeft (180).reduced (0, 8));
    top.removeFromLeft (16);
    detectorLabel.setBounds (top.removeFromLeft (64));
    detectorBox.setBounds (top.removeFromLeft (110).reduced (0, 8));

    area.removeFromTop (8);

    juce::Array<LabeledKnob*> visible;
    for (auto* k : { &threshold, &ratio, &attack, &release, &makeup, &depth, &lookahead })
        if (k->slider.isVisible())
            visible.add (k);

    if (visible.isEmpty())
        return;

    auto row = area.removeFromTop (juce::jmin (150, area.getHeight()));
    const int w = row.getWidth() / visible.size();
    for (auto* k : visible)
    {
        auto cell = row.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (18));
        k->slider.setBounds (cell.reduced (4));
    }
}
