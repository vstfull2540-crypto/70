#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class ArmNongMuangAnalyzerAudioProcessor final : public juce::AudioProcessor
{
public:
    ArmNongMuangAnalyzerAudioProcessor();
    ~ArmNongMuangAnalyzerAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> spectrum[256];
    std::atomic<float> peakSpectrum[256];
    std::atomic<float> leftRms { 0.0f }, rightRms { 0.0f }, correlation { 0.0f };
    std::atomic<float> scopeX { 0.0f }, scopeY { 0.0f };
    std::atomic<bool> clipping { false };

    // Real-time FFT analyzer buffers; audio is passed through unchanged.
    static constexpr int fftOrder = 13; // 8192-point FFT improves low-frequency wavelet resolution
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> fftWindow { fftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, fftSize> fftFifo {};
    std::array<float, fftSize * 2> fftData {};
    int fftFifoIndex = 0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::dsp::IIR::Filter<float> lowShelfL, lowShelfR, midPeakL, midPeakR, highShelfL, highShelfR;
    // Per-channel, per-band envelope followers for sample-based compression.
    float envLowL = 0.0f, envLowR = 0.0f;
    float envMidL = 0.0f, envMidR = 0.0f;
    float envHighL = 0.0f, envHighR = 0.0f;
    juce::dsp::LinkwitzRileyFilter<float> lowSplitL, lowSplitR, highSplitL, highSplitR;
    double currentSampleRate = 44100.0;
    float envL = 0.0f, envR = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArmNongMuangAnalyzerAudioProcessor)
};
