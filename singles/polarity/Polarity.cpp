#include "Shell.h"

// Polarity aus SpaceX: L und R getrennt umpolen, weich ueberblendet (kein
// Knacksen). Ohne PRE/POST - es gibt hier nichts davor oder dahinter.
namespace
{
    constexpr auto ID_ON = "on", ID_L = "flipL", ID_R = "flipR";

    juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_ON, 1 }, "On", true));
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_L, 1 }, "Polarity L", false));
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_R, 1 }, "Polarity R", false));
        return { p.begin(), p.end() };
    }
}

class PolarityProcessor : public fairy::ShellProcessor
{
public:
    PolarityProcessor() : ShellProcessor (layout(), false, "POLARITY") {}

    void prepareToPlay (double sampleRate, int) override
    {
        for (auto* s : { &onS, &lS, &rS })
            s->reset (sampleRate, 0.03);
        onS.setCurrentAndTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        lS.setCurrentAndTargetValue (param (ID_L) > 0.5f ? 1.0f : 0.0f);
        rS.setCurrentAndTargetValue (param (ID_R) > 0.5f ? 1.0f : 0.0f);
        meterFifo.clear();
        setLatencySamples (0);
    }

    void processStereo (float* l, float* r, int n) override
    {
        onS.setTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        lS.setTargetValue  (param (ID_L)  > 0.5f ? 1.0f : 0.0f);
        rS.setTargetValue  (param (ID_R)  > 0.5f ? 1.0f : 0.0f);
        for (int i = 0; i < n; ++i)
        {
            const float g = onS.getNextValue(), fl = lS.getNextValue(), fr = rS.getNextValue();
            l[i] *= 1.0f - 2.0f * g * fl;
            r[i] *= 1.0f - 2.0f * g * fr;
        }
    }

    juce::AudioProcessorEditor* createEditor() override;

private:
    juce::SmoothedValue<float> onS, lS, rS;
};

class PolarityEditor : public fairy::ShellEditor
{
public:
    explicit PolarityEditor (PolarityProcessor& p) : ShellEditor (p, "POLARITY", ID_ON, 250)
    {
        for (auto* b : { &flipL, &flipR })
        {
            b->setClickingTogglesState (false);
            b->setWantsKeyboardFocus (false);
            b->getProperties().set ("thinOnFrame", true);
            b->getProperties().set ("softChip", true);
            b->getProperties().set ("altAccent", true);
            b->getProperties().set ("phaseIcon", true);
            b->getProperties().set ("phaseScale", 1.3);
            addControl (*b);
        }
        flipL.setButtonText ("L");
        flipR.setButtonText ("R");
        // Klick = umschalten, Cmd-Klick = nur diese Seite (wie in SpaceX).
        auto click = [this] (bool left)
        {
            if (juce::ModifierKeys::currentModifiers.isCommandDown())
            {
                setParam (ID_L, left ? 1.0f : 0.0f);
                setParam (ID_R, left ? 0.0f : 1.0f);
            }
            else
            {
                const char* id = left ? ID_L : ID_R;
                setParam (id, shell.param (id) > 0.5f ? 0.0f : 1.0f);
            }
            refreshNow();
        };
        flipL.onClick = [click] { click (true); };
        flipR.onClick = [click] { click (false); };

        link.setClickingTogglesState (false);
        link.setWantsKeyboardFocus (false);
        link.getProperties().set ("linkIcon", true);
        link.getProperties().set ("noLinkLines", true);
        addControl (link);
        link.onClick = [this]
        {
            const bool both = shell.param (ID_L) > 0.5f && shell.param (ID_R) > 0.5f;
            setParam (ID_L, both ? 0.0f : 1.0f);
            setParam (ID_R, both ? 0.0f : 1.0f);
            refreshNow();
        };
        finishSetup();
    }
    ~PolarityEditor() override { clearAttachments(); }

private:
    void layoutCard (juce::Rectangle<int> inner, juce::Rectangle<int>) override
    {
        // Masse wie in SpaceX: 56 x 32, mal 1,15 / 1,3; Luecke 28, Link 20.
        constexpr int lrW = 56, lrH = 32, gap = 28;
        const int bigW = juce::roundToInt (lrW * 1.15f), bigH = juce::roundToInt (lrH * 1.3f);
        const int cx = inner.getCentreX(), cy = inner.getCentreY() - 6;
        link.setBounds (cx - 10, cy - 10, 20, 20);
        flipL.setBounds (cx - gap / 2 - bigW, cy - bigH / 2, bigW, bigH);
        flipR.setBounds (cx + gap / 2,        cy - bigH / 2, bigW, bigH);
    }

    void syncControls (bool) override
    {
        const bool l = shell.param (ID_L) > 0.5f, r = shell.param (ID_R) > 0.5f;
        if (flipL.getToggleState() != l) flipL.setToggleState (l, juce::dontSendNotification);
        if (flipR.getToggleState() != r) flipR.setToggleState (r, juce::dontSendNotification);
        if (link.getToggleState() != (l && r)) { link.setToggleState (l && r, juce::dontSendNotification); link.repaint(); }
    }

    juce::TextButton flipL, flipR, link;
};

juce::AudioProcessorEditor* PolarityProcessor::createEditor() { return new PolarityEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PolarityProcessor(); }
