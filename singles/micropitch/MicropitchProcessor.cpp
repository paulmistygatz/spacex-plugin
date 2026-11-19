#include "MicropitchProcessor.h"
#include "MicropitchEditor.h"

MicropitchProcessor::MicropitchProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MICROPITCH", createLayout())
{
    pOn     = apvts.getRawParameterValue (ID_ON);
    pMode   = apvts.getRawParameterValue (ID_MODE);
    pAmount = apvts.getRawParameterValue (ID_AMOUNT);
}

juce::AudioProcessorValueTreeState::ParameterLayout MicropitchProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    // Wertebereiche und Defaults wie in SpaceX (Parallax Mode / Amount / Drift On).
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_ON, 1 }, "On", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ID_MODE, 1 }, "Mode",
        juce::StringArray { "Velvet", "Halo", "Illusion", "Double" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_AMOUNT, 1 }, "Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));
    return { params.begin(), params.end() };
}

bool MicropitchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo rein/raus - und Mono rein, Stereo raus: genau dafuer ist
    // Micropitch da (ein Mono-Signal breit machen).
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo()
        && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void MicropitchProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    meterFifo.clear();
    setLatencySamples (0);
}

void MicropitchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);
    if (buffer.getNumChannels() < 2 || n <= 0)
        return;
    if (numIn == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);   // Mono -> beide Seiten

    engine.setParameters (pOn->load() > 0.5f,
                          juce::jlimit (0, micropitch::kModes - 1, (int) std::round (pMode->load())),
                          pAmount->load() * 0.01f);
    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);
    engine.process (l, r, n);

    if (meterActive.load (std::memory_order_relaxed))
        meterFifo.push (l, r, n);
}

juce::AudioProcessorEditor* MicropitchProcessor::createEditor()
{
    return new MicropitchEditor (*this);
}

void MicropitchProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MicropitchProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicropitchProcessor();
}
