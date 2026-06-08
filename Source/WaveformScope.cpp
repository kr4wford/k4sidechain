#include "WaveformScope.h"

WaveformScope::WaveformScope (K4SideChainProcessor& p) : processor (p)
{
    setOpaque (true);
    scratchDry.assign (4096, 0.0f);
    scratchWet.assign (4096, 0.0f);
    startTimerHz (60);
}

WaveformScope::~WaveformScope()
{
    stopTimer();
}

void WaveformScope::pushColumn (float dryPeak, float wetPeak)
{
    dryPeaks[head] = dryPeak;
    wetPeaks[head] = wetPeak;
    head = (head + 1) % history;
}

void WaveformScope::timerCallback()
{
    if (paused)
        return;

    // Drain everything available, tracking the peak magnitude of this frame.
    float dPk = 0.0f, wPk = 0.0f;
    int got;
    do
    {
        got = processor.readVisualizerSamples (scratchDry.data(), scratchWet.data(),
                                               (int) scratchDry.size());
        for (int i = 0; i < got; ++i)
        {
            dPk = juce::jmax (dPk, std::abs (scratchDry[i]));
            wPk = juce::jmax (wPk, std::abs (scratchWet[i]));
        }
    } while (got == (int) scratchDry.size());

    // One new column per frame keeps the scroll speed steady and the look clean.
    pushColumn (dPk, wPk);
    repaint();
}

void WaveformScope::mouseDown (const juce::MouseEvent&)
{
    paused = ! paused;
    repaint();
}

void WaveformScope::mouseEnter (const juce::MouseEvent&)
{
    hovering = true;
    repaint();
}

void WaveformScope::mouseExit (const juce::MouseEvent&)
{
    hovering = false;
    repaint();
}

void WaveformScope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Background.
    g.setGradientFill (juce::ColourGradient (
        juce::Colour (0xff15151c), b.getCentreX(), b.getY(),
        juce::Colour (0xff0c0c10), b.getCentreX(), b.getBottom(), false));
    g.fillAll();

    const float mid    = b.getCentreY();
    const float halfH  = b.getHeight() * 0.42f;
    const float w      = b.getWidth();

    // Centre line.
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) mid, b.getX(), b.getRight());

    const auto peakAt = [this] (const std::array<float, history>& a, int col)
    {
        // col 0 = oldest (left), col history-1 = newest (right)
        const int idx = (head + col) % history;
        return juce::jlimit (0.0f, 1.0f, a[(size_t) idx]);
    };

    // Build a mirrored, filled waveform path from a peak array.
    const auto buildPath = [&] (const std::array<float, history>& a)
    {
        juce::Path path;
        path.startNewSubPath (b.getX(), mid);
        for (int c = 0; c < history; ++c)
        {
            const float x = b.getX() + (w * c) / (history - 1);
            path.lineTo (x, mid - peakAt (a, c) * halfH);
        }
        for (int c = history - 1; c >= 0; --c)
        {
            const float x = b.getX() + (w * c) / (history - 1);
            path.lineTo (x, mid + peakAt (a, c) * halfH);
        }
        path.closeSubPath();
        return path;
    };

    // Dry silhouette underneath (shows the original, un-ducked level).
    g.setColour (juce::Colour (0xff3a86ff).withAlpha (0.22f));
    g.fillPath (buildPath (dryPeaks));

    // Wet signal on top (the actual ducked output).
    auto wetPath = buildPath (wetPeaks);
    g.setColour (juce::Colour (0xff4cc2ff).withAlpha (0.85f));
    g.fillPath (wetPath);
    g.setColour (juce::Colour (0xffaee3ff));
    g.strokePath (wetPath, juce::PathStrokeType (1.0f));

    // Hover hint / pause indicator.
    if (paused)
    {
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (juce::String::fromUTF8 ("❙❙  paused"),
                    getLocalBounds().reduced (10).removeFromBottom (22),
                    juce::Justification::centredRight);
    }
    else if (hovering)
    {
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("click to pause",
                    getLocalBounds().reduced (10).removeFromBottom (20),
                    juce::Justification::centredRight);
    }
}
