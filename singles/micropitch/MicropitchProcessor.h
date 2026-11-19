#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "MicropitchDsp.h"
#include "MeterFifo.h"

class MicropitchProcessor : public juce::AudioProcessor
{
public:
    static constexpr auto ID_ON     = "on";
    static constexpr auto ID_MODE   = "mode";
    static constexpr auto ID_AMOUNT = "amount";

    MicropitchProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override { engine.reset(); }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.03; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Goniometer: die Ausgangssamples, nur solange das Meter offen ist.
    mistycat::MeterFifo meterFifo;
    std::atomic<bool>   meterActive { false };

    // Aufgeklappt oder nicht - wird mit dem Projekt gespeichert.
    static constexpr auto kMeterOpenProp = "meterOpen";
    static constexpr auto kUiScaleProp   = "uiScale";     // Fenstergroesse (1.0 = 304 bzw. 524 x 240)
    bool isMeterOpen() const     { return apvts.state.getProperty (kMeterOpenProp, false); }
    void setMeterOpen (bool b)   { apvts.state.setProperty (kMeterOpenProp, b, nullptr); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    micropitch::Engine engine;
    std::atomic<float>* pOn = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pAmount = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicropitchProcessor)
};
