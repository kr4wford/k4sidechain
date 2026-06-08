#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

/**
    A clean, scrolling waveform background.

    Draws the dry (input) signal as a faint silhouette with the wet (sidechained)
    signal layered brightly on top, so the gap between them shows exactly how much
    the sidechain is ducking the audio. Click anywhere on it to pause / resume.
*/
class WaveformScope : public juce::Component,
                      private juce::Timer
{
public:
    explicit WaveformScope (K4SideChainProcessor&);
    ~WaveformScope() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void pushColumn (float dryPeak, float wetPeak);

    static constexpr int history = 600; // number of columns kept

    K4SideChainProcessor& processor;

    std::array<float, history> dryPeaks {};
    std::array<float, history> wetPeaks {};
    int  head = 0;          // ring index where the next column is written
    bool paused = false;
    bool hovering = false;

    std::vector<float> scratchDry, scratchWet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformScope)
};
