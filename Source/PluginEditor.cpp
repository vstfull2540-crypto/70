#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
    juce::Image loadPng(const void* data, int size)
    {
        return juce::ImageFileFormat::loadFrom(data, static_cast<size_t>(size));
    }

    const juce::Colour spectrumColour { 0xff22eaff };
    const juce::Colour peakColour { 0xffffa52b };
    const juce::Colour scopeColour { 0xff1685ff };
}

ArmNongMuangAnalyzerAudioProcessorEditor::ArmNongMuangAnalyzerAudioProcessorEditor(
    ArmNongMuangAnalyzerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    background = loadPng(AnalyzerPng::ArmNongMuang_Analyzer_UI_png,
                         AnalyzerPng::ArmNongMuang_Analyzer_UI_pngSize);

    // The artwork is 1414x1113. Render the complete interface at exactly 50% scale.
    setResizable(false, false);
    setSize(referenceWidth / 2, referenceHeight / 2);

    auto add = [this](juce::TextButton& b, juce::Rectangle<int> bounds, std::function<void()> action)
    {
        addHitButton(b, bounds, std::move(action));
    };

    // Lower-right analyzer controls. Each transparent hit target has its own non-overlapping bounds.
    add(lfButton,       { 838, 568, 84, 44 }, [this] {
        const float current = audioProcessor.parameters.getRawParameterValue("lowfreq")->load();
        const float next = current > 30.0f ? 20.0f : (current > 15.0f ? 10.0f : 40.0f);
        setParameter("lowfreq", next);
    });
    add(weightButton,   { 961, 568, 83, 44 }, [this] {
        cycleParameter("weight", 0.0f, 3.0f, 1.0f);
    });
    add(freezeButton,   { 838, 650, 84, 44 }, [this] {
        const bool on = audioProcessor.parameters.getRawParameterValue("freeze")->load() > 0.5f;
        setParameter("freeze", on ? 0.0f : 1.0f);
    });
    add(showButton,     { 961, 650, 83, 44 }, [this] {
        const bool on = audioProcessor.parameters.getRawParameterValue("show")->load() > 0.5f;
        setParameter("show", on ? 0.0f : 1.0f);
    });
    add(peakButton,     { 838, 733, 84, 44 }, [this] {
        const bool on = audioProcessor.parameters.getRawParameterValue("peakhold")->load() > 0.5f;
        setParameter("peakhold", on ? 0.0f : 1.0f);
    });
    add(clearButton,    { 961, 733, 83, 44 }, [this] {
        for (auto& v : audioProcessor.peakSpectrum) v.store(0.0f);
    });
    add(detectButton,   { 838, 816, 84, 44 }, [this] {
        const bool rms = audioProcessor.parameters.getRawParameterValue("detectrms")->load() > 0.5f;
        setParameter("detectrms", rms ? 0.0f : 1.0f);
    });
    add(responseButton, { 961, 816, 83, 44 }, [this] {
        cycleParameter("response", 40.0f, 1000.0f, 40.0f);
    });
    add(saveDataButton, { 838, 899, 238, 38 }, [this] { saveGraphMemory(); });
    add(saveButton,     { 1105, 899, 236, 38 }, [this] { storeSnapshot(snapshotA, hasSnapshotA); });
    add(graphMemButton, { 838, 945, 238, 38 }, [this] {
        if (hasGraphMemory)
            for (size_t i = 0; i < graphMemory.size(); ++i)
                audioProcessor.peakSpectrum[i].store(graphMemory[i]);
    });
    add(clearMemButton, { 1105, 945, 236, 38 }, [this] { clearGraphMemory(); });

    // Header: preset field, previous/next, A/B snapshots, and menu/bypass.
    add(presetButton,         { 421, 24, 332, 48 }, [this] { cycleParameter("response", 40.0f, 1000.0f, 40.0f); });
    add(previousPresetButton, { 756, 24, 47, 48 }, [this] { cycleParameter("response", 40.0f, 1000.0f, -40.0f); });
    add(nextPresetButton,     { 808, 24, 47, 48 }, [this] { cycleParameter("response", 40.0f, 1000.0f, 40.0f); });
    add(snapshotAButton,      { 870, 24, 46, 48 }, [this] {
        if (hasSnapshotA) recallSnapshot(snapshotA, hasSnapshotA);
        else storeSnapshot(snapshotA, hasSnapshotA);
    });
    add(snapshotBButton,      { 921, 24, 46, 48 }, [this] {
        if (hasSnapshotB) recallSnapshot(snapshotB, hasSnapshotB);
        else storeSnapshot(snapshotB, hasSnapshotB);
    });
    add(menuButton,           { 974, 24, 49, 48 }, [this] {
        const bool on = audioProcessor.parameters.getRawParameterValue("show")->load() > 0.5f;
        setParameter("show", on ? 0.0f : 1.0f);
    });

    // Navigation arrows and zoom controls, aligned to the PNG artwork.
    add(navigateUp,    { 1217, 158, 36, 34 }, [this] { panOffsetY = juce::jmin(0.4f, panOffsetY + 0.05f); });
    add(navigateDown,  { 1217, 232, 36, 28 }, [this] { panOffsetY = juce::jmax(-0.4f, panOffsetY - 0.05f); });
    add(navigateLeft,  { 1171, 192, 31, 39 }, [this] { panOffset = juce::jmax(-0.4f, panOffset - 0.05f); });
    add(navigateRight, { 1269, 192, 31, 39 }, [this] { panOffset = juce::jmin(0.4f, panOffset + 0.05f); });
    add(navigateCentre,{ 1204, 193, 65, 38 }, [this] { panOffset = 0.0f; panOffsetY = 0.0f; });
    add(zoomIn,        { 1202, 329, 68, 30 }, [this] { zoomFactor = juce::jmin(4.0f, zoomFactor * 1.2f); });
    add(zoomOut,       { 1202, 413, 68, 30 }, [this] { zoomFactor = juce::jmax(1.0f, zoomFactor / 1.2f); });
    add(resetZoom,     { 1284, 462, 43, 47 }, [this] { zoomFactor = 1.0f; panOffset = 0.0f; panOffsetY = 0.0f; });

    startTimerHz(30);
    resized();
}

ArmNongMuangAnalyzerAudioProcessorEditor::~ArmNongMuangAnalyzerAudioProcessorEditor()
{
    stopTimer();
}

void ArmNongMuangAnalyzerAudioProcessorEditor::addHitButton(
    juce::TextButton& button, juce::Rectangle<int> bounds, std::function<void()> action)
{
    button.setButtonText({});
    button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    button.setLookAndFeel(nullptr);
    button.setAlpha(0.0f);
    referenceBounds[&button] = bounds;
    button.setBounds(bounds);
    button.onClick = [this, &button, action = std::move(action)]() mutable
    {
        highlightedButton = &button;
        action();
        repaint();
    };
    addAndMakeVisible(button);
}

void ArmNongMuangAnalyzerAudioProcessorEditor::setParameter(const juce::String& id, float value)
{
    if (auto* parameter = audioProcessor.parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void ArmNongMuangAnalyzerAudioProcessorEditor::cycleParameter(
    const juce::String& id, float minimum, float maximum, float step)
{
    const float current = audioProcessor.parameters.getRawParameterValue(id)->load();
    float next = current + step;
    if (step < 0.0f && next < minimum - 0.0001f)
        next = maximum;
    else if (step >= 0.0f && next > maximum + 0.0001f)
        next = minimum;
    setParameter(id, next);
}

void ArmNongMuangAnalyzerAudioProcessorEditor::saveGraphMemory()
{
    for (size_t i = 0; i < graphMemory.size(); ++i)
        graphMemory[i] = audioProcessor.spectrum[i].load();
    hasGraphMemory = true;
}

void ArmNongMuangAnalyzerAudioProcessorEditor::clearGraphMemory()
{
    graphMemory.fill(0.0f);
    hasGraphMemory = false;
    for (auto& v : audioProcessor.peakSpectrum)
        v.store(0.0f);
}

void ArmNongMuangAnalyzerAudioProcessorEditor::storeSnapshot(std::array<float, 256>& target, bool& valid)
{
    for (size_t i = 0; i < target.size(); ++i)
        target[i] = audioProcessor.spectrum[i].load();
    valid = true;
}

void ArmNongMuangAnalyzerAudioProcessorEditor::recallSnapshot(const std::array<float, 256>& source, bool valid)
{
    if (!valid)
        return;
    for (size_t i = 0; i < source.size(); ++i)
        audioProcessor.peakSpectrum[i].store(source[i]);
}

void ArmNongMuangAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    const float sxScale = static_cast<float>(getWidth()) / static_cast<float>(referenceWidth);
    const float syScale = static_cast<float>(getHeight()) / static_cast<float>(referenceHeight);
    g.addTransform(juce::AffineTransform::scale(sxScale, syScale));
    if (background.isValid())
        g.drawImage(background, juce::Rectangle<float>(0.0f, 0.0f, (float) referenceWidth, (float) referenceHeight));

    // The PNG is the only visible interface. Dynamic data is drawn only inside its
    // graph/meter windows so no duplicate panels, labels or controls are introduced.
    const juce::Rectangle<float> plot { 98.0f, 132.0f, 990.0f, 334.0f };
    if (audioProcessor.parameters.getRawParameterValue("show")->load() > 0.5f)
    {
        juce::Path spectrumPath, peakPath;
        constexpr int displayedWaveletBands = 68;
        for (int i = 0; i < displayedWaveletBands; ++i)
        {
            const float normalizedIndex = static_cast<float>(i) / static_cast<float>(displayedWaveletBands - 1);
            const int dataIndex = juce::jlimit(0, 255, juce::roundToInt(normalizedIndex * 255.0f));
            const float shifted = (normalizedIndex - 0.5f) / zoomFactor + 0.5f + panOffset;
            const float x = plot.getX() + shifted * plot.getWidth();
            float value = audioProcessor.spectrum[dataIndex].load();
            float peak = audioProcessor.peakSpectrum[dataIndex].load();
            if (hasGraphMemory)
                peak = juce::jmax(peak, graphMemory[static_cast<size_t>(dataIndex)]);
            // Visual spectrum boost only: raise the displayed trace without changing audio gain.
            constexpr float spectrumDisplayBoost = 1.60f;
            const float boostedValue = value * spectrumDisplayBoost;
            const float boostedPeak = peak * spectrumDisplayBoost;
            const float y = plot.getBottom() - juce::jlimit(0.0f, 1.0f, boostedValue + panOffsetY) * plot.getHeight();
            const float py = plot.getBottom() - juce::jlimit(0.0f, 1.0f, boostedPeak + panOffsetY) * plot.getHeight();
            if (i == 0) { spectrumPath.startNewSubPath(x, y); peakPath.startNewSubPath(x, py); }
            else { spectrumPath.lineTo(x, y); peakPath.lineTo(x, py); }
        }
        g.saveState();
        g.reduceClipRegion(plot.toNearestInt());
        g.setColour(spectrumColour);
        g.strokePath(spectrumPath, juce::PathStrokeType(2.0f));
        g.setColour(peakColour);
        g.strokePath(peakPath, juce::PathStrokeType(1.3f));
        g.restoreState();
    }

    // Stereo vectorscope overlay in the semicircular graph region.
    const juce::Rectangle<float> scope { 34.0f, 546.0f, 744.0f, 324.0f };
    g.saveState();
    g.reduceClipRegion(scope.toNearestInt());
    g.setColour(scopeColour.withAlpha(0.85f));
    const float centerX = scope.getCentreX();
    const float baseY = scope.getBottom() - 1.0f;
    for (int ring = 1; ring <= 4; ++ring)
    {
        const float radius = static_cast<float>(ring) * scope.getWidth() / 8.0f;
        g.drawEllipse(centerX - radius, baseY - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    }
    g.drawVerticalLine(static_cast<int>(centerX), scope.getY(), baseY);
    g.drawLine(scope.getX() + 30.0f, scope.getY() + 20.0f, centerX, baseY, 1.0f);
    g.drawLine(centerX, baseY, scope.getRight() - 30.0f, scope.getY() + 20.0f, 1.0f);

    const float sx = audioProcessor.scopeX.load();
    const float sy = audioProcessor.scopeY.load();
    juce::Path trace;
    constexpr int tracePoints = 72;
    for (int i = 0; i < tracePoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(tracePoints - 1);
        const float x = centerX + (sx * 0.85f) * (scope.getWidth() * 0.46f) * std::sin(t * juce::MathConstants<float>::twoPi);
        const float y = baseY - juce::jlimit(0.0f, 1.0f, std::abs(sy) + 0.08f) * (scope.getHeight() * 0.95f) * t;
        if (i == 0) trace.startNewSubPath(x, y); else trace.lineTo(x, y);
    }
    g.setColour(spectrumColour);
    g.strokePath(trace, juce::PathStrokeType(1.8f));
    g.restoreState();

    // Stereo level bars fit inside the PNG's existing meter windows.
    const float left = audioProcessor.leftRms.load();
    const float right = audioProcessor.rightRms.load();
    // Three meter lanes are aligned to the three wells in the supplied PNG:
    // left channel, combined stereo RMS, and right channel.
    // Move all three live meter fills up 10 reference pixels from Fix14. Move Stereo RMS and
    // Right RMS 10 reference pixels left as requested. The PNG background is unchanged.
    const juce::Rectangle<float> leftMeter   { 1177.0f, 623.0f, 27.0f, 250.0f };
    const juce::Rectangle<float> stereoMeter { 1228.0f, 623.0f, 27.0f, 250.0f };
    const juce::Rectangle<float> rightMeter  { 1284.0f, 623.0f, 27.0f, 250.0f };
    const float stereoRms = std::sqrt(0.5f * (left * left + right * right));
    // Increase the meter's visual sensitivity: map -72 dBFS to the bottom and
    // 0 dBFS to the top. This changes only the display scale, never audio gain.
    auto drawMeter = [&g](juce::Rectangle<float> r, float rms)
    {
        const float dbfs = juce::Decibels::gainToDecibels(juce::jmax(0.0f, rms), -100.0f);
        const float normalized = juce::jlimit(0.0f, 1.0f, (dbfs + 72.0f) / 72.0f);
        const float fillHeight = normalized * r.getHeight();
        r.setY(r.getBottom() - fillHeight);
        r.setHeight(fillHeight);
        g.setColour(juce::Colour(0xff18d9ff).withAlpha(0.82f));
        g.fillRect(r);
    };
    drawMeter(leftMeter, left);
    drawMeter(stereoMeter, stereoRms);
    drawMeter(rightMeter, right);

    // Tiny state indicators are drawn only within existing button artwork.
    if (audioProcessor.parameters.getRawParameterValue("bypass")->load() > 0.5f)
    {
        g.setColour(juce::Colours::red.withAlpha(0.85f));
        g.fillEllipse(1330.0f, 26.0f, 12.0f, 12.0f);
    }

    // The hit targets stay invisible, but the last clicked control is clearly
    // outlined in yellow so users can see exactly which button was activated.
    if (highlightedButton != nullptr)
    {
        const auto found = referenceBounds.find(highlightedButton);
        if (found != referenceBounds.end())
        {
            g.setColour(juce::Colour(0xffffdf36));
            g.drawRoundedRectangle(found->second.toFloat().reduced(2.0f), 4.0f, 3.0f);
        }
    }
}

void ArmNongMuangAnalyzerAudioProcessorEditor::resized()
{
    const float sx = static_cast<float>(getWidth()) / static_cast<float>(referenceWidth);
    const float sy = static_cast<float>(getHeight()) / static_cast<float>(referenceHeight);
    for (const auto& entry : referenceBounds)
    {
        const auto b = entry.second;
        entry.first->setBounds(juce::roundToInt(b.getX() * sx), juce::roundToInt(b.getY() * sy),
                               juce::jmax(1, juce::roundToInt(b.getWidth() * sx)),
                               juce::jmax(1, juce::roundToInt(b.getHeight() * sy)));
    }
}

void ArmNongMuangAnalyzerAudioProcessorEditor::timerCallback()
{
    repaint();
}
