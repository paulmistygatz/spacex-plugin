#include "SidesEqUi.h"
#include "SidesEqFilter.h"
#include "LCRExtractor.h"   // aus SpaceX (../Source/DSP)

// LCR aus SpaceX (Sektion "LCR Matrix") mit dem Sides EQ in der Stellung
// "EQ -> LCR": der EQ wirkt auf Center (Mitten-Kurve) und die getrennten
// L/R-Seiten (Seiten-Kurve). Rechenweg aus LCRMSAudioProcessor::processBlock.
namespace
{
    constexpr auto ID_ON = "on", ID_LCR = "lcr", ID_CWEIGHT = "cWeight", ID_REGAIN = "regain",
                   ID_EQ = "eqMode", ID_EQ_AMT = "eqAmount";

    juce::String regainText (double v)
    {
        if (v < 0.5) return "Off";
        const double hz = 20000.0 * std::pow (0.025, v * 0.01);
        if (hz >= 1000.0) return juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz";
        return juce::String ((int) std::round (hz)) + " Hz";
    }

    juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_ON, 1 }, "On", true));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_LCR, 1 }, "LCR",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_CWEIGHT, 1 }, "C-Weight",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_REGAIN, 1 }, "HF Regain",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                             [] (float v, int) { return regainText (v); })));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ID_EQ, 1 }, "Sides EQ",
                         juce::StringArray { "Tight", "Clear", "Focus", "Flat" }, 3));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_EQ_AMT, 1 }, "Sides EQ Amount",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));
        return { p.begin(), p.end() };
    }
}

class LcrProcessor : public fairy::ShellProcessor
{
public:
    LcrProcessor() : ShellProcessor (layout(), false, "LCR") {}

    void prepareToPlay (double sampleRate, int) override
    {
        sr = sampleRate;
        extractor.prepare (sampleRate);
        latency = extractor.getLatencySamples();
        setLatencySamples (latency);   // fest - auch wenn die Sektion aus ist
        dryL.assign ((size_t) juce::jmax (1, latency), 0.0f);
        dryR.assign ((size_t) juce::jmax (1, latency), 0.0f);
        dryPos = 0;
        sensS.reset (sampleRate, 0.02);  sensS.setCurrentAndTargetValue (param (ID_CWEIGHT) * 0.01f);
        blendS.reset (sampleRate, 0.02); blendS.setCurrentAndTargetValue (param (ID_LCR) * 0.01f);
        wetGain.reset (sampleRate, 0.03);
        wetGain.setCurrentAndTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        paused = false;
        warmup = 0;
        eq.prepare (sampleRate, (int) std::round (param (ID_EQ)), param (ID_EQ_AMT) * 0.01f);
        eqC = {}; eqL.clear(); eqR.clear();
        meterFifo.clear();
    }

    void reset() override
    {
        extractor.reset();
        std::fill (dryL.begin(), dryL.end(), 0.0f);
        std::fill (dryR.begin(), dryR.end(), 0.0f);
        eqC = {}; eqL.clear(); eqR.clear();
    }

    void processStereo (float* left, float* right, int n) override
    {
        // Ein/Aus mit Aufwaermphase wie in SpaceX: nach dem Einschalten erst
        // drei Latenzlaengen sauber einlaufen lassen, dann einblenden.
        if (param (ID_ON) <= 0.5f)
        {
            wetGain.setTargetValue (0.0f);
            warmup = 0;
            if (! paused && ! wetGain.isSmoothing() && wetGain.getCurrentValue() <= 0.0f)
                paused = true;
        }
        else
        {
            if (paused)
            {
                paused = false;
                extractor.reset();
                warmup = 3 * latency;
            }
            if (warmup > 0) { wetGain.setTargetValue (0.0f); warmup -= n; }
            else            wetGain.setTargetValue (1.0f);
        }

        sensS.setTargetValue (param (ID_CWEIGHT) * 0.01f);
        blendS.setTargetValue (param (ID_LCR) * 0.01f);
        {
            const float air = param (ID_REGAIN);
            extractor.setExtractionRange (120.0f, air < 0.5f ? 96000.0f : 20000.0f * std::pow (0.025f, air * 0.01f));
        }
        const bool eqOn = eq.beginBlock ((int) std::round (param (ID_EQ)), param (ID_EQ_AMT) * 0.01f, n);
        if (! eqOn) { eqC = {}; eqL.clear(); eqR.clear(); }

        const int dSize = (int) dryL.size();
        for (int i = 0; i < n; ++i)
        {
            const float l = left[i], r = right[i];
            const float sens  = sensS.getNextValue();
            const float blend = blendS.getNextValue();
            const float gC = 1.0f - blend;   // LCR senkt nur das Center, die Seiten bleiben

            float lOnly = 0.0f, center = 0.0f, rOnly = 0.0f;
            if (! paused)
                extractor.processSample (l, r, sens, lOnly, center, rOnly);
            const float wg = wetGain.getNextValue();
            if (eqOn)
            {
                center = eqC.process (center, eq.mHs);
                lOnly  = eqL.process (lOnly, eq);
                rOnly  = eqR.process (rOnly, eq);
            }
            const float wetL = lOnly + center * gC;
            const float wetR = rOnly + center * gC;

            const float dl = dryL[(size_t) dryPos], dr = dryR[(size_t) dryPos];
            dryL[(size_t) dryPos] = l;
            dryR[(size_t) dryPos] = r;
            dryPos = (dryPos + 1) % dSize;

            left[i]  = dl + (wetL - dl) * wg;
            right[i] = dr + (wetR - dr) * wg;
        }
    }

    juce::AudioProcessorEditor* createEditor() override;

private:
    double sr = 48000.0;
    StereoSTFTExtractor extractor;
    int latency = 0;
    std::vector<float> dryL, dryR;
    int dryPos = 0;
    juce::SmoothedValue<float> sensS, blendS, wetGain;
    bool paused = false;
    int  warmup = 0;
    fairy::SidesEqCoeffs eq;
    fairy::Biquad eqC;
    fairy::SideChainEq eqL, eqR;
};

class LcrEditor : public fairy::ShellEditor
{
public:
    explicit LcrEditor (LcrProcessor& p) : ShellEditor (p, "LCR", ID_ON, 470)
    {
        bars.setSliderStyle (juce::Slider::LinearVertical);
        bars.setSliderSnapsToMousePosition (false);
        bars.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        bars.getProperties().set ("focusStyle", true);
        bars.getProperties().set ("lcrBars", true);
        addControl (bars);
        barsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ID_LCR, bars);
        bars.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
        addLabel (barsLabel, "LCR");
        auto barsText = [this]
        {
            barsLabel.setText (bars.isMouseButtonDown() ? juce::String (juce::roundToInt (bars.getValue())) + " %" : juce::String ("LCR"),
                               juce::dontSendNotification);
        };
        bars.onValueChange = barsText; bars.onDragStart = barsText; bars.onDragEnd = barsText;

        addKnob (cWeight, cWeightLabel, "C-Weight", ID_CWEIGHT, 50.0,
                 [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; });
        cWeight.getProperties().set ("centerOut", true);
        addKnob (regain, regainLabel, "HF Regain", ID_REGAIN, 0.0, [] (double v) { return regainText (v); });

        eqUi.setup (*this, content, p, ID_EQ, ID_EQ_AMT, [this] (juce::Component& c) { addControl (c); }, eqAmtAttachment);
        glide.onFrame = [this] { eqUi.tickIcon(); };
        finishSetup();
    }
    ~LcrEditor() override { clearAttachments(); barsAttachment.reset(); eqAmtAttachment.reset(); }

private:
    void layoutCard (juce::Rectangle<int> inner, juce::Rectangle<int> header) override
    {
        fairy::SidesEqUi::placeFader (header, eqUi.amount);

        constexpr int knobD = 96;
        const int barsW = juce::roundToInt (juce::jlimit (10.0f, 18.0f, (float) knobD * 0.19f) * 3.9f);
        constexpr int fieldW = 118;
        const int g = (inner.getWidth() - barsW - 2 * knobD - fieldW) / 4;
        const int y = inner.getY() + (inner.getHeight() - (knobD + kLabelH)) / 2;
        int x = inner.getX() + g / 2;

        const int labelY = y + knobD;
        bars.setBounds (x, y - 7, barsW, labelY + 2 - (y - 7));
        bars.setMouseDragSensitivity (juce::jmax (20, bars.getHeight() - 8));
        barsLabel.setBounds (x - g / 2, labelY, barsW + g, kLabelH);
        x += barsW + g;
        placeKnob ({ x, y, knobD, knobD }, y, knobD, cWeight, cWeightLabel);
        x += knobD + g;
        placeKnob ({ x, y, knobD, knobD }, y, knobD, regain, regainLabel);
        x += knobD + g;
        placeModeField ({ x, y, fieldW, knobD }, y + knobD / 2, eqUi.field, eqUi.dots);
    }

    void syncControls (bool on) override
    {
        eqUi.sync (on);
        // C-Weight zeigt sich an den Balkenbreiten (SpaceX Runde 182).
        const float cw = (float) cWeight.getValue() * 0.01f;
        if (std::abs ((float) bars.getProperties().getWithDefault ("cWeight", 0.5f) - cw) > 0.002f)
        {
            bars.getProperties().set ("cWeight", cw);
            bars.repaint();
        }
    }

    juce::Slider bars, cWeight, regain;
    juce::Label  barsLabel, cWeightLabel, regainLabel;
    fairy::SidesEqUi eqUi;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> barsAttachment, eqAmtAttachment;
};

juce::AudioProcessorEditor* LcrProcessor::createEditor() { return new LcrEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LcrProcessor(); }
