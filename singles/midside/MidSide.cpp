#include "SidesEqUi.h"
#include "SidesEqFilter.h"

// Mid-Side aus SpaceX: Width und Sides (Boost) auf der Seite, darunter
// 120 Hz fest geschuetzt (Bass Guard), dazu der Sides EQ.
// Rechenweg aus LCRMSAudioProcessor::processBlock (Abschnitt 3).
namespace
{
    constexpr auto ID_ON = "on", ID_WIDTH = "width", ID_SIDES = "sides", ID_EQ = "eqMode", ID_EQ_AMT = "eqAmount";

    juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_ON, 1 }, "On", true));
        // 50..200 %, Skew so, dass 100 % genau auf 12 Uhr steht (wie SpaceX).
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_WIDTH, 1 }, "Width",
                         juce::NormalisableRange<float> (50.0f, 200.0f, 0.1f, 0.6309f), 100.0f, "%"));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_SIDES, 1 }, "Sides",
                         juce::NormalisableRange<float> (0.0f, 6.0f, 0.01f), 0.0f, "dB"));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ID_EQ, 1 }, "Sides EQ",
                         juce::StringArray { "Tight", "Clear", "Focus", "Flat" }, 3));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_EQ_AMT, 1 }, "Sides EQ Amount",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));
        return { p.begin(), p.end() };
    }
}

class MidSideProcessor : public fairy::ShellProcessor
{
public:
    MidSideProcessor() : ShellProcessor (layout(), false, "MIDSIDE") {}

    void prepareToPlay (double sampleRate, int) override
    {
        widthS.reset (sampleRate, 0.02); widthS.setCurrentAndTargetValue (param (ID_WIDTH) * 0.01f);
        boostS.reset (sampleRate, 0.02); boostS.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (param (ID_SIDES)));
        onGain.reset (sampleRate, 0.03); onGain.setCurrentAndTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        fairy::highpassCoeffs (guardC, sampleRate, 120.0f);
        guard = {};
        eq.prepare (sampleRate, (int) std::round (param (ID_EQ)), param (ID_EQ_AMT) * 0.01f);
        eqM = {}; eqS.clear();
        meterFifo.clear();
        setLatencySamples (0);
    }
    void reset() override { guard = {}; eqM = {}; eqS.clear(); }

    void processStereo (float* left, float* right, int n) override
    {
        widthS.setTargetValue (param (ID_WIDTH) * 0.01f);
        boostS.setTargetValue (juce::Decibels::decibelsToGain (param (ID_SIDES)));
        onGain.setTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        const bool eqOn = eq.beginBlock ((int) std::round (param (ID_EQ)), param (ID_EQ_AMT) * 0.01f, n);
        if (! eqOn) { eqM = {}; eqS.clear(); }

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i], r = right[i];
            const float width = widthS.getNextValue();
            const float boost = boostS.getNextValue();
            const float g     = onGain.getNextValue();
            const float m = 0.5f * (l + r);
            const float s = 0.5f * (l - r);
            const float sGuard = guard.process (s, guardC);           // unter 120 Hz bleibt die Seite unveraendert
            const float sWet = s + sGuard * (boost * width - 1.0f);
            float mEq = m, sEq = sWet;
            if (eqOn)
            {
                mEq = eqM.process (m, eq.mHs);
                sEq = eqS.process (sWet, eq);
            }
            const float lWet = mEq + sEq, rWet = mEq - sEq;
            left[i]  = l + (lWet - l) * g;
            right[i] = r + (rWet - r) * g;
        }
    }

    juce::AudioProcessorEditor* createEditor() override;

private:
    juce::SmoothedValue<float> widthS, boostS, onGain;
    fairy::Biquad::C guardC;
    fairy::Biquad guard;
    fairy::SidesEqCoeffs eq;
    fairy::Biquad eqM;
    fairy::SideChainEq eqS;
};

class MidSideEditor : public fairy::ShellEditor
{
public:
    explicit MidSideEditor (MidSideProcessor& p) : ShellEditor (p, "MID-SIDE", ID_ON, 390)
    {
        addKnob (width, widthLabel, "Width", ID_WIDTH, 100.0,
                 [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; });
        width.getProperties().set ("centerOut", true);
        width.getProperties().set ("widthWedge", true);
        addKnob (sides, sidesLabel, "Sides", ID_SIDES, 0.0,
                 [] (double v) { return "+" + juce::String (v, 1) + " dB"; });
        eqUi.setup (*this, content, p, ID_EQ, ID_EQ_AMT, [this] (juce::Component& c) { addControl (c); }, eqAmtAttachment);
        glide.onFrame = [this] { eqUi.tickIcon(); };
        finishSetup();
    }
    ~MidSideEditor() override { clearAttachments(); eqAmtAttachment.reset(); }

private:
    void layoutCard (juce::Rectangle<int> inner, juce::Rectangle<int> header) override
    {
        fairy::SidesEqUi::placeFader (header, eqUi.amount);
        constexpr int knobD = 96, fieldW = 118;
        const int g = (inner.getWidth() - 2 * knobD - fieldW) / 4;
        const int y = inner.getY() + (inner.getHeight() - (knobD + kLabelH)) / 2;
        int x = inner.getX() + g / 2;
        placeKnob ({ x, y, knobD, knobD }, y, knobD, width, widthLabel);
        x += knobD + g;
        placeKnob ({ x, y, knobD, knobD }, y, knobD, sides, sidesLabel);
        x += knobD + g;
        placeModeField ({ x, y, fieldW, knobD }, y + knobD / 2, eqUi.field, eqUi.dots);
    }
    void syncControls (bool on) override { eqUi.sync (on); }

    juce::Slider width, sides;
    juce::Label  widthLabel, sidesLabel;
    fairy::SidesEqUi eqUi;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqAmtAttachment;
};

juce::AudioProcessorEditor* MidSideProcessor::createEditor() { return new MidSideEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MidSideProcessor(); }
