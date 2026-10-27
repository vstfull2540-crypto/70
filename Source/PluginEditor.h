#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <unordered_map>

class ArmNongMuangAnalyzerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                       private juce::Timer
{
public:
    explicit ArmNongMuangAnalyzerAudioProcessorEditor(ArmNongMuangAnalyzerAudioProcessor&);
    ~ArmNongMuangAnalyzerAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void addHitButton(juce::TextButton&, juce::Rectangle<int>, std::function<void()>);
    void setParameter(const juce::String&, float);
    void cycleParameter(const juce::String&, float, float, float);
    void saveGraphMemory();
    void clearGraphMemory();
    void storeSnapshot(std::array<float, 256>& target, bool& valid);
    void recallSnapshot(const std::array<float, 256>& source, bool valid);

    ArmNongMuangAnalyzerAudioProcessor& audioProcessor;
    juce::Image background;
    juce::ImageComponent backgroundComponent;

    // These controls are transparent hit targets over the PNG artwork. They never draw
    // labels, borders or widgets over the reference artwork.
    juce::TextButton lfButton, weightButton, freezeButton, showButton, peakButton, clearButton;
    juce::TextButton detectButton, responseButton, saveDataButton, saveButton, graphMemButton, clearMemButton;
    juce::TextButton bypassButton, autoEqButton, multibandButton, presetButton, previousPresetButton, nextPresetButton;
    juce::TextButton snapshotAButton, snapshotBButton, menuButton, navigateCentre;
    juce::TextButton navigateUp, navigateDown, navigateLeft, navigateRight, zoomIn, zoomOut, resetZoom;
    std::array<float, 256> graphMemory {}, snapshotA {}, snapshotB {};
    bool hasGraphMemory = false, hasSnapshotA = false, hasSnapshotB = false;
    std::unordered_map<juce::Component*, juce::Rectangle<int>> referenceBounds;
    juce::Component* highlightedButton = nullptr; // Last clicked transparent hit target.
    static constexpr int referenceWidth = 1414, referenceHeight = 1113;
    float zoomFactor = 1.0f;
    float panOffset = 0.0f, panOffsetY = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArmNongMuangAnalyzerAudioProcessorEditor)
};
