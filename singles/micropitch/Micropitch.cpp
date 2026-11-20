#include "Shell.h"
#include "MicropitchDsp.h"

namespace
{
    constexpr auto ID_ON = "on", ID_MODE = "mode", ID_AMOUNT = "amount";

    juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        // Bereiche und Defaults wie in SpaceX (Drift On / Parallax Mode / Parallax Amount).
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_ON, 1 }, "On", true));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ID_MODE, 1 }, "Mode",
                         juce::StringArray { "Velvet", "Halo", "Illusion", "Double" }, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_AMOUNT, 1 }, "Amount",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));
        return { p.begin(), p.end() };
    }
}

class MicropitchProcessor : public fairy::ShellProcessor
{
public:
    MicropitchProcessor() : ShellProcessor (layout(), true, "MICROPITCH") {}

    void prepareToPlay (double sr, int) override { engine.prepare (sr); meterFifo.clear(); setLatencySamples (0); }
    void reset() override { engine.reset(); }

    void processStereo (float* l, float* r, int n) override
    {
        engine.setParameters (param (ID_ON) > 0.5f,
                              juce::jlimit (0, micropitch::kModes - 1, (int) std::round (param (ID_MODE))),
                              param (ID_AMOUNT) * 0.01f);
        engine.process (l, r, n);
    }

    juce::AudioProcessorEditor* createEditor() override;

private:
    micropitch::Engine engine;
};

class MicropitchEditor : public fairy::ShellEditor
{
public:
    explicit MicropitchEditor (MicropitchProcessor& p) : ShellEditor (p, "MICROPITCH", ID_ON, 280)
    {
        addKnob (amount, amountLabel, "Amount", ID_AMOUNT, 0.0,
                 [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; });
        addModeField (mode, modeDots, "pxDiagram", micropitch::kModes);
        mode.onClick = [this]
        {
            const bool back = juce::ModifierKeys::currentModifiers.isCommandDown();
            setParam (ID_MODE, (float) ((choice (ID_MODE) + (back ? micropitch::kModes - 1 : 1)) % micropitch::kModes));
            syncOnState();
        };
        modeDots.onPick = [this] (int i) { setParam (ID_MODE, (float) i); syncOnState(); };
        finishSetup();
    }
    ~MicropitchEditor() override { clearAttachments(); }

private:
    void layoutCard (juce::Rectangle<int> inner, juce::Rectangle<int>) override
    {
        constexpr int knobD = 96;
        const int y = inner.getY() + (inner.getHeight() - (knobD + kLabelH)) / 2;
        auto left = inner.removeFromLeft (inner.getWidth() / 2);
        placeKnob (left, y, knobD, amount, amountLabel);
        placeModeField (inner, y + knobD / 2, mode, modeDots);
    }

    void syncControls (bool on) override
    {
        static const char* const names[micropitch::kModes] = { "VELVET", "HALO", "ILLUSION", "DOUBLE" };
        const int m = juce::jlimit (0, micropitch::kModes - 1, choice (ID_MODE));
        if (mode.getButtonText() != names[m]) mode.setButtonText (names[m]);
        if ((int) mode.getProperties()["pxDiagram"] != m) { mode.getProperties().set ("pxDiagram", m); mode.repaint(); }
        if (modeDots.index != m || modeDots.off == on) { modeDots.index = m; modeDots.off = ! on; modeDots.repaint(); }
    }

    juce::Slider amount;
    juce::Label  amountLabel;
    juce::TextButton mode;
    fairy::ModeDots modeDots;
};

juce::AudioProcessorEditor* MicropitchProcessor::createEditor() { return new MicropitchEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MicropitchProcessor(); }
