#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;
    juce::dsp::IIR::Coefficients<float>::Ptr makePeak(double sr, float freq, float q, float gainDb)
    {
        return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, freq, q,
                    juce::Decibels::decibelsToGain(gainDb));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ArmNongMuangAnalyzerAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<APB>("bypass", "Bypass", false));
    p.add(std::make_unique<APB>("autoeq", "Auto EQ", true));
    p.add(std::make_unique<APB>("multiband", "Multiband", true));
    p.add(std::make_unique<APF>("intensity", "Intensity", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.85f));
    p.add(std::make_unique<APF>("q", "Q", juce::NormalisableRange<float>(0.3f, 4.0f, 0.01f), 1.4f));
    p.add(std::make_unique<APF>("threshold", "Threshold", juce::NormalisableRange<float>(-36.0f, 0.0f, 0.1f), -18.0f));
    p.add(std::make_unique<APF>("ratio", "Ratio", juce::NormalisableRange<float>(1.0f, 12.0f, 0.1f), 3.0f));
    p.add(std::make_unique<APF>("output", "Output", juce::NormalisableRange<float>(-12.0f, 6.0f, 0.1f), 0.0f));
    p.add(std::make_unique<APF>("lowfreq", "LF res", juce::NormalisableRange<float>(10.0f, 160.0f, 1.0f), 40.0f));
    p.add(std::make_unique<APB>("freeze", "Freeze", false));
    p.add(std::make_unique<APB>("show", "Show Spectrum", true));
    p.add(std::make_unique<APB>("peakhold", "Peak Hold", true));
    p.add(std::make_unique<APC>("weight", "Weight", juce::StringArray { "Flat", "A", "B", "C" }, 0));
    p.add(std::make_unique<APB>("detectrms", "Detect RMS", true));
    p.add(std::make_unique<APF>("response", "Response", juce::NormalisableRange<float>(40.0f, 1000.0f, 40.0f), 160.0f));
    return p;
}

ArmNongMuangAnalyzerAudioProcessor::ArmNongMuangAnalyzerAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& v : spectrum) v.store(0.0f);
    for (auto& v : peakSpectrum) v.store(0.0f);
}

void ArmNongMuangAnalyzerAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    currentSampleRate = sr;
    fftFifoIndex = 0;
    fftFifo.fill(0.0f);
    fftData.fill(0.0f);
    const juce::dsp::ProcessSpec spec {
        sr,
        static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock)),
        static_cast<juce::uint32>(juce::jmax(1, getTotalNumOutputChannels()))
    };
    for (auto* f : { &lowShelfL, &lowShelfR, &midPeakL, &midPeakR, &highShelfL, &highShelfR }) f->reset();
    lowSplitL.prepare(spec); lowSplitR.prepare(spec); highSplitL.prepare(spec); highSplitR.prepare(spec);
    lowSplitL.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
    lowSplitR.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
    highSplitL.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
    highSplitR.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
    lowSplitL.setCutoffFrequency(160.0f); lowSplitR.setCutoffFrequency(160.0f);
    highSplitL.setCutoffFrequency(2500.0f); highSplitR.setCutoffFrequency(2500.0f);
    envLowL = envLowR = envMidL = envMidR = envHighL = envHighR = 0.0f;
}

void ArmNongMuangAnalyzerAudioProcessor::releaseResources() {}

bool ArmNongMuangAnalyzerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void ArmNongMuangAnalyzerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int inputChannels = getTotalNumInputChannels();
    const int outputChannels = getTotalNumOutputChannels();
    const int samples = buffer.getNumSamples();
    for (int ch = inputChannels; ch < outputChannels; ++ch)
        buffer.clear(ch, 0, samples);

    // PAZ-style analyzer behavior: never alter the incoming audio samples.
    // Measurements are calculated from the input and do not affect the signal path.
    double sumL = 0.0, sumR = 0.0, cross = 0.0;
    float blockPeak = 0.0f;
    const bool freeze = parameters.getRawParameterValue("freeze")->load() > 0.5f;
    const bool detectRms = parameters.getRawParameterValue("detectrms")->load() > 0.5f;
    const int weighting = juce::jlimit(0, 3, juce::roundToInt(parameters.getRawParameterValue("weight")->load()));
    const float responseMs = juce::jmax(40.0f, parameters.getRawParameterValue("response")->load());
    const float sampleRate = static_cast<float>(juce::jmax(1.0, currentSampleRate));

    for (int i = 0; i < samples; ++i)
    {
        const float l = inputChannels > 0 ? buffer.getSample(0, i) : 0.0f;
        const float r = inputChannels > 1 ? buffer.getSample(1, i) : l;
        blockPeak = juce::jmax(blockPeak, juce::jmax(std::abs(l), std::abs(r)));
        sumL += static_cast<double>(l) * l;
        sumR += static_cast<double>(r) * r;
        cross += static_cast<double>(l) * r;

        scopeX.store((l - r) * 0.5f, std::memory_order_relaxed);
        scopeY.store((l + r) * 0.5f, std::memory_order_relaxed);

        if (!freeze)
        {
            fftFifo[static_cast<size_t>(fftFifoIndex++)] = (l + r) * 0.5f;
            if (fftFifoIndex >= fftSize)
            {
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                std::copy(fftFifo.begin(), fftFifo.end(), fftData.begin());
                fftWindow.multiplyWithWindowingTable(fftData.data(), fftSize);
                fft.performFrequencyOnlyForwardTransform(fftData.data());
                fftFifoIndex = 0;

                const float nyquist = sampleRate * 0.5f;
                const float maxHz = juce::jmin(20000.0f, nyquist);
                const float minHz = juce::jmin(20.0f, maxHz * 0.5f);
                // 68-band Morlet-style wavelet filter bank (constant-Q, log-spaced).
                // Each band is the energy response of a Gaussian frequency-domain
                // wavelet centered on its own frequency; this is not a 1/3-octave bank.
                constexpr int waveletBandCount = 68;
                std::array<float, waveletBandCount> bandLevels {};
                constexpr float waveletQ = 8.0f;
                const float logRange = std::log(maxHz / minHz);
                for (int band = 0; band < waveletBandCount; ++band)
                {
                    const float position = static_cast<float>(band) / static_cast<float>(waveletBandCount - 1);
                    const float centreHz = minHz * std::exp(logRange * position);
                    const float sigmaHz = juce::jmax(2.0f, centreHz / waveletQ);
                    double weightedEnergy = 0.0;
                    double weightSum = 0.0;
                    for (int k = 1; k < fftSize / 2; ++k)
                    {
                        const float binHz = static_cast<float>(k) * sampleRate / static_cast<float>(fftSize);
                        const float distance = (binHz - centreHz) / sigmaHz;
                        if (std::abs(distance) > 4.0f)
                            continue;
                        const float waveletWeight = std::exp(-0.5f * distance * distance);
                        const float magnitude = fftData[static_cast<size_t>(k)] / (static_cast<float>(fftSize) * 0.5f);
                        weightedEnergy += static_cast<double>(waveletWeight) * magnitude * magnitude;
                        weightSum += waveletWeight;
                    }
                    const float magnitude = weightSum > 1.0e-12
                        ? static_cast<float>(std::sqrt(weightedEnergy / weightSum)) : 0.0f;

                    const float f2 = centreHz * centreHz;
                    const float f4 = f2 * f2;
                    const float c1 = 20.6f * 20.6f;
                    const float c2 = 107.7f * 107.7f;
                    const float c3 = 737.9f * 737.9f;
                    const float c4 = 158.5f * 158.5f;
                    const float c5 = 12194.0f * 12194.0f;
                    float weightDb = 0.0f;
                    if (weighting == 1)
                    {
                        const float ra = (c5 * f4) / ((f2 + c1) * std::sqrt((f2 + c2) * (f2 + c3)) * (f2 + c5));
                        weightDb = 20.0f * std::log10(juce::jmax(1.0e-12f, ra)) + 2.0f;
                    }
                    else if (weighting == 2)
                    {
                        const float rb = (c5 * centreHz * f2) / ((f2 + c1) * std::sqrt(f2 + c4) * (f2 + c5));
                        weightDb = 20.0f * std::log10(juce::jmax(1.0e-12f, rb)) + 0.17f;
                    }
                    else if (weighting == 3)
                    {
                        const float rc = (c5 * f2) / ((f2 + c1) * (f2 + c5));
                        weightDb = 20.0f * std::log10(juce::jmax(1.0e-12f, rc)) + 0.06f;
                    }
                    const float db = juce::jlimit(-80.0f, 0.0f,
                        juce::Decibels::gainToDecibels(magnitude, -100.0f) + weightDb);
                    bandLevels[static_cast<size_t>(band)] = (db + 80.0f) / 80.0f;
                }

                // Preserve the existing 256-point drawing/memory interface by resampling
                // the 68 genuine wavelet bands for smooth rendering and existing controls.
                const float frameSeconds = static_cast<float>(fftSize) / sampleRate;
                const float smoothing = 1.0f - std::exp(-frameSeconds / (responseMs * 0.001f));
                for (int b = 0; b < 256; ++b)
                {
                    const float bandPosition = static_cast<float>(b) * (waveletBandCount - 1) / 255.0f;
                    const int band0 = juce::jlimit(0, waveletBandCount - 1, static_cast<int>(bandPosition));
                    const int band1 = juce::jmin(waveletBandCount - 1, band0 + 1);
                    const float fraction = bandPosition - static_cast<float>(band0);
                    const float normalized = bandLevels[static_cast<size_t>(band0)] * (1.0f - fraction)
                                           + bandLevels[static_cast<size_t>(band1)] * fraction;
                    const float old = spectrum[b].load(std::memory_order_relaxed);
                    const float smoothed = detectRms ? old + smoothing * (normalized - old)
                                                     : juce::jmax(normalized, old * 0.985f);
                    spectrum[b].store(smoothed, std::memory_order_relaxed);
                    if (parameters.getRawParameterValue("peakhold")->load() > 0.5f)
                    {
                        const float previousPeak = peakSpectrum[b].load(std::memory_order_relaxed);
                        peakSpectrum[b].store(juce::jmax(previousPeak, normalized), std::memory_order_relaxed);
                    }
                    else
                    {
                        peakSpectrum[b].store(smoothed, std::memory_order_relaxed);
                    }
                }
            }
        }
    }

    const float rmsL = samples > 0 ? static_cast<float>(std::sqrt(sumL / samples)) : 0.0f;
    const float rmsR = samples > 0 ? static_cast<float>(std::sqrt(sumR / samples)) : 0.0f;
    leftRms.store(rmsL, std::memory_order_relaxed);
    rightRms.store(rmsR, std::memory_order_relaxed);
    correlation.store((sumL > 1.0e-12 && sumR > 1.0e-12)
        ? static_cast<float>(cross / std::sqrt(sumL * sumR)) : 0.0f, std::memory_order_relaxed);
    clipping.store(blockPeak >= 0.999f, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ArmNongMuangAnalyzerAudioProcessor::createEditor()
{
    return new ArmNongMuangAnalyzerAudioProcessorEditor(*this);
}
void ArmNongMuangAnalyzerAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}
void ArmNongMuangAnalyzerAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArmNongMuangAnalyzerAudioProcessor();
}
