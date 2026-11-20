#include "Shell.h"
#include "FastMath.h"   // aus SpaceX (../Source/DSP)

// Phaser aus SpaceX (Sektion "Phaser", intern RAYE): 4 Allpass-Stufen pro
// Seite mit Rueckkopplung, vier Charaktere, FAST = 30 % schneller.
// LINK (Kopplung an Autopan) gibt es hier nicht - die Rate ist die freie
// SpaceX-Rate. Rechenweg aus LCRMSAudioProcessor::processBlock (RAY).
namespace
{
    constexpr auto ID_ON = "on", ID_AMOUNT = "amount", ID_CHAR = "character", ID_FAST = "fast";

    struct Character { float centreHz, sweepMul, fbMul, mixMul, stereoOffset, rateMul; };
    const Character kChars[4] = { {  800.0f, 1.00f, 1.00f, 1.00f, 0.25f,  1.00f },    // Sweep
                                  { 4500.0f, 0.45f, 0.70f, 1.00f, 0.25f,  2.50f },    // Shimmer
                                  {  380.0f, 1.45f, 1.45f, 1.10f, 0.50f,  0.70f },    // Spin
                                  { 1600.0f, 1.20f, 0.90f, 1.00f, 0.125f, 0.35f } };  // Swirl

    juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_ON, 1 }, "On", true));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_AMOUNT, 1 }, "Amount",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ID_CHAR, 1 }, "Character",
                         juce::StringArray { "Sweep", "Shimmer", "Spin", "Swirl" }, 0));
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_FAST, 1 }, "Fast", false));
        return { p.begin(), p.end() };
    }
}

class PhaserProcessor : public fairy::ShellProcessor
{
public:
    PhaserProcessor() : ShellProcessor (layout(), true, "PHASER") {}

    void prepareToPlay (double sampleRate, int) override
    {
        sr = sampleRate;
        onGain.reset (sampleRate, 0.03); onGain.setCurrentAndTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        depthS.reset (sampleRate, 0.02); depthS.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, param (ID_AMOUNT) * 0.01f));
        spacex::sinTable();
        clearStates();
        phase = 0.0;
        meterFifo.clear();
        setLatencySamples (0);
    }
    void reset() override { clearStates(); }

    void processStereo (float* left, float* right, int n) override
    {
        onGain.setTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        depthS.setTargetValue (juce::jlimit (0.0f, 1.0f, param (ID_AMOUNT) * 0.01f));
        const auto& ch = kChars[juce::jlimit (0, 3, (int) std::round (param (ID_CHAR)))];
        const double rayCycle = 1.0 / 0.15;
        const double fastMul  = param (ID_FAST) > 0.5f ? 1.3 : 1.0;
        const double phaseInc = (1.0 / rayCycle) / sr * (double) ch.rateMul * fastMul;
        const float piOverFs  = juce::MathConstants<float>::pi / (float) sr;

        for (int i = 0; i < n; ++i)
        {
            float l = left[i], r = right[i];
            const float gain     = onGain.getNextValue();
            const float depthRaw = depthS.getNextValue();
            const float offFade  = juce::jlimit (0.0f, 1.0f, depthRaw * 6.0f);
            const float depth01  = juce::jlimit (0.0f, 1.0f, depthRaw);
            if (gain > 0.0001f && offFade > 0.0005f)
            {
                statesClear = false;
                const double ph = phase;
                const double stereoOff = (double) ch.stereoOffset;
                const float sweepOct = (1.3f + depth01 * 1.7f) * ch.sweepMul;
                const float feedback = (0.04f + depth01 * 0.20f) * ch.fbMul;
                const float mix      = (0.10f + depth01 * 0.28f) * ch.mixMul;
                auto coeffFor = [&] (float lfo) -> float
                {
                    const float f = ch.centreHz * std::exp2 (0.5f * sweepOct * lfo);
                    const float t = std::tan (juce::jlimit (30.0f, 0.45f * (float) sr, f) * piOverFs);
                    return (t - 1.0f) / (t + 1.0f);
                };
                if (coefCountdown <= 0)
                {
                    constexpr int kStep = 8;
                    if (! coefValid)
                    {
                        aL = coeffFor (spacex::fastSinCycles (ph));
                        aR = coeffFor (spacex::fastSinCycles (ph + stereoOff));
                        coefValid = true;
                    }
                    const double phNext = ph + phaseInc * (double) kStep;
                    stepL = (coeffFor (spacex::fastSinCycles (phNext)) - aL) / (float) kStep;
                    stepR = (coeffFor (spacex::fastSinCycles (phNext + stereoOff)) - aR) / (float) kStep;
                    coefCountdown = kStep;
                }
                const float cL = aL, cR = aR;
                aL += stepL; aR += stepR;
                --coefCountdown;

                float xL = l + fbL * feedback;
                float xR = r + fbR * feedback;
                for (int k = 0; k < kStages; ++k)
                {
                    const float yL = cL * xL + apL[k]; apL[k] = xL - cL * yL; xL = yL;
                    const float yR = cR * xR + apR[k]; apR[k] = xR - cR * yR; xR = yR;
                }
                fbL = juce::jlimit (-2.0f, 2.0f, xL);
                fbR = juce::jlimit (-2.0f, 2.0f, xR);
                const float wetL = l + (xL - l) * mix;
                const float wetR = r + (xR - r) * mix;
                l = l + (wetL - l) * gain * offFade;
                r = r + (wetR - r) * gain * offFade;
            }
            else if (! statesClear)
                clearStates();
            phase += phaseInc;
            if (phase >= 1.0) phase -= 1.0;
            left[i] = l; right[i] = r;
        }
    }

    juce::AudioProcessorEditor* createEditor() override;

private:
    void clearStates()
    {
        for (int k = 0; k < kStages; ++k) { apL[k] = 0.0f; apR[k] = 0.0f; }
        fbL = fbR = 0.0f;
        coefValid = false;
        coefCountdown = 0;
        statesClear = true;
    }

    static constexpr int kStages = 4;
    double sr = 48000.0, phase = 0.0;
    float apL[kStages] {}, apR[kStages] {}, fbL = 0.0f, fbR = 0.0f;
    float aL = 0.0f, aR = 0.0f, stepL = 0.0f, stepR = 0.0f;
    int coefCountdown = 0;
    bool coefValid = false, statesClear = true;
    juce::SmoothedValue<float> onGain, depthS;
};

class PhaserEditor : public fairy::ShellEditor
{
public:
    explicit PhaserEditor (PhaserProcessor& p) : ShellEditor (p, "PHASER", ID_ON, 280)
    {
        addKnob (amount, amountLabel, "Amount", ID_AMOUNT, 0.0,
                 [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; });
        addModeField (character, dots, "rayDiagram", 4);
        character.onClick = [this]
        {
            const bool back = juce::ModifierKeys::currentModifiers.isCommandDown();
            setParam (ID_CHAR, (float) ((choice (ID_CHAR) + (back ? 3 : 1)) % 4));
            refreshNow();
        };
        dots.onPick = [this] (int i) { setParam (ID_CHAR, (float) i); refreshNow(); };

        fast.setButtonText ("FAST");
        fast.setClickingTogglesState (true);
        fast.setWantsKeyboardFocus (false);
        fast.getProperties().set ("thinOnFrame", true);
        fast.getProperties().set ("altAccent", true);
        fast.getProperties().set ("softChip", true);
        fast.getProperties().set ("hdrIcon", 1);
        fast.getProperties().set ("hdrTextSize", 11.0);
        fast.getProperties().set ("ctlStyle", 1);
        addControl (fast);
        fastAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (shell.apvts, ID_FAST, fast);
        finishSetup();
    }
    ~PhaserEditor() override { clearAttachments(); fastAttachment.reset(); }

private:
    void layoutCard (juce::Rectangle<int> inner, juce::Rectangle<int> header) override
    {
        const int w = CustomLookAndFeel::ctlContentWidth (fast);
        fast.setBounds (header.removeFromRight (w).withSizeKeepingCentre (w, 24));

        constexpr int knobD = 96;
        const int y = inner.getY() + (inner.getHeight() - (knobD + kLabelH)) / 2;
        auto left = inner.removeFromLeft (inner.getWidth() / 2);
        placeKnob (left, y, knobD, amount, amountLabel);
        placeModeField (inner, y + knobD / 2, character, dots);
    }

    void syncControls (bool on) override
    {
        static const char* const names[4] = { "SWEEP", "SHIMMER", "SPIN", "SWIRL" };
        const int c = juce::jlimit (0, 3, choice (ID_CHAR));
        if (character.getButtonText() != names[c]) character.setButtonText (names[c]);
        if ((int) character.getProperties()["rayDiagram"] != c) { character.getProperties().set ("rayDiagram", c); character.repaint(); }
        if (dots.index != c || dots.off == on) { dots.index = c; dots.off = ! on; dots.repaint(); }
    }

    juce::Slider amount;
    juce::Label  amountLabel;
    juce::TextButton character, fast;
    fairy::ModeDots dots;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fastAttachment;
};

juce::AudioProcessorEditor* PhaserProcessor::createEditor() { return new PhaserEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PhaserProcessor(); }
