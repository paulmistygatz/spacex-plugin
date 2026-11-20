#include "Shell.h"
#include "FastMath.h"   // aus SpaceX (../Source/DSP)

// Autopan aus SpaceX (Sektion "Autopan"): Sinus oder geglaetteter Puls,
// freie Rate in Hz oder taktsynchron. Constant-Power-Pan.
// Rechenweg aus LCRMSAudioProcessor::processBlock (Abschnitt 4).
namespace
{
    constexpr auto ID_ON = "on", ID_AMOUNT = "amount", ID_SPEED = "speed", ID_SYNC = "sync",
                   ID_RATE = "syncRate", ID_PULSE = "pulse";
    const float kBarValues[] = { 1.0f/16.0f, 1.0f/8.0f, 1.0f/4.0f, 1.0f/2.0f, 1.0f, 2.0f, 4.0f, 8.0f };

    juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_ON, 1 }, "On", true));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_AMOUNT, 1 }, "Amount",
                         juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ID_SPEED, 1 }, "Speed",
                         juce::NormalisableRange<float> (0.02f, 8.0f, 0.001f, 0.35f), 0.25f, "Hz"));
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_SYNC, 1 }, "Sync", false));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ID_RATE, 1 }, "Sync Rate",
                         juce::StringArray { "1/16", "1/8", "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 6));
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ID_PULSE, 1 }, "Pulse", false));
        return { p.begin(), p.end() };
    }
}

class AutopanProcessor : public fairy::ShellProcessor
{
public:
    AutopanProcessor() : ShellProcessor (layout(), true, "AUTOPAN") {}

    void prepareToPlay (double sampleRate, int) override
    {
        sr = sampleRate;
        phase = 0.0;
        pulseState = 0.0f;
        pulseCoeff = std::exp (-1.0f / (0.020f * (float) sampleRate));   // ~20 ms
        moveS.reset (sampleRate, 0.02);  moveS.setCurrentAndTargetValue (param (ID_AMOUNT) * 0.01f);
        onGain.reset (sampleRate, 0.03); onGain.setCurrentAndTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);
        spacex::sinTable();
        meterFifo.clear();
        setLatencySamples (0);
    }

    void processStereo (float* left, float* right, int n) override
    {
        moveS.setTargetValue (param (ID_AMOUNT) * 0.01f);
        onGain.setTargetValue (param (ID_ON) > 0.5f ? 1.0f : 0.0f);

        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto hostBpm = pos->getBpm())
                    if (*hostBpm > 1.0)
                        bpm = *hostBpm;
        double cycleSeconds;
        if (param (ID_SYNC) > 0.5f)
        {
            const int idx = juce::jlimit (0, 7, (int) std::round (param (ID_RATE)));
            cycleSeconds = juce::jmax (0.001, (double) kBarValues[idx] * (60.0 / bpm) * 4.0);
        }
        else
            cycleSeconds = 1.0 / juce::jmax (0.01, (double) param (ID_SPEED));
        const double inc = (1.0 / cycleSeconds) * (1.0 / sr);
        const bool pulse = param (ID_PULSE) > 0.5f;

        for (int i = 0; i < n; ++i)
        {
            const float move = moveS.getNextValue();
            const float g = onGain.getNextValue();
            float l = left[i], r = right[i];
            float lF = l, rF = r;
            if (move > 0.0001f)
            {
                float lfo;
                if (pulse)
                {
                    const float target = (phase < 0.5) ? 1.0f : -1.0f;
                    pulseState = target + pulseCoeff * (pulseState - target);
                    lfo = pulseState;
                }
                else
                    lfo = spacex::fastSinCycles (phase);
                const float panPos = juce::jlimit (-1.0f, 1.0f, lfo * move);
                const double turns = (double) (panPos + 1.0f) * 0.125;
                lF = l * spacex::fastCosCycles (turns) * juce::MathConstants<float>::sqrt2;
                rF = r * spacex::fastSinCycles (turns) * juce::MathConstants<float>::sqrt2;
            }
            left[i]  = l + (lF - l) * g;
            right[i] = r + (rF - r) * g;
            phase += inc;
            if (phase >= 1.0) phase -= 1.0;
        }
    }

    juce::AudioProcessorEditor* createEditor() override;

private:
    double sr = 48000.0, phase = 0.0;
    float pulseState = 0.0f, pulseCoeff = 0.0f;
    juce::SmoothedValue<float> moveS, onGain;
};

// Speed-Regler, der bei Sync gesperrt ist (Cmd-Klick setzt trotzdem zurueck).
struct SyncLockSlider : public juce::Slider
{
    bool locked() const { return (bool) getProperties().getWithDefault ("syncLocked", false); }
    void mouseDown (const juce::MouseEvent& e) override { if (locked() && ! e.mods.isCommandDown()) return; juce::Slider::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (locked()) return; juce::Slider::mouseDrag (e); }
    void mouseUp   (const juce::MouseEvent& e) override { if (locked() && ! e.mods.isCommandDown()) return; juce::Slider::mouseUp (e); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { if (locked()) return; juce::Slider::mouseWheelMove (e, w); }
};

class AutopanEditor : public fairy::ShellEditor
{
public:
    explicit AutopanEditor (AutopanProcessor& p) : ShellEditor (p, "AUTOPAN", ID_ON, 400)
    {
        addKnob (amount, amountLabel, "Amount", ID_AMOUNT, 0.0,
                 [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; });
        amount.getProperties().set ("movementRing", true);
        addKnob (speed, speedLabel, "Speed", ID_SPEED, 0.25,
                 [] (double v) { return juce::String (v, v < 1.0 ? 2 : 1) + " Hz"; });

        auto iconButton = [this] (juce::TextButton& b, const char* kind, const char* id,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& att)
        {
            b.setClickingTogglesState (true);
            b.setWantsKeyboardFocus (false);
            b.getProperties().set (kind, true);
            b.getProperties().set ("glowIcon", true);
            addControl (b);
            att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (shell.apvts, id, b);
        };
        iconButton (pulseButton, "pulseIcon", ID_PULSE, pulseAttachment);
        iconButton (syncButton,  "syncIcon",  ID_SYNC,  syncAttachment);
        syncButton.onClick = [this] { refreshNow(); };

        rateBox.addItemList ({ "1/16", "1/8", "1/4", "1/2", "1 BAR", "2 BARS", "4 BARS", "8 BARS" }, 1);
        rateBox.getProperties().set ("noArrow", true);
        addControl (rateBox);
        rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (shell.apvts, ID_RATE, rateBox);
        finishSetup();
    }
    ~AutopanEditor() override
    {
        clearAttachments();
        pulseAttachment.reset(); syncAttachment.reset(); rateAttachment.reset();
    }

private:
    void layoutCard (juce::Rectangle<int> inner, juce::Rectangle<int>) override
    {
        // Reihenfolge wie in SpaceX: Amount, Puls, Speed, Sync, Takt.
        constexpr int moveD = 96, speedD = 80, iconS = 28, gap = 12, boxW = 88, boxH = 30;
        const int total = moveD + gap + iconS + gap + speedD + gap + iconS + 10 + boxW;
        int x = inner.getX() + (inner.getWidth() - total) / 2;
        const int y = inner.getY() + (inner.getHeight() - (moveD + kLabelH)) / 2;
        const int cy = y + moveD / 2;
        placeKnob ({ x, y, moveD, moveD }, y, moveD, amount, amountLabel);
        x += moveD + gap;
        pulseButton.setBounds (x, cy - iconS / 2, iconS, iconS);
        x += iconS + gap;
        // Speed etwas kleiner, Beschriftung auf derselben Linie wie Amount.
        placeKnob ({ x, y, speedD, speedD }, y + (moveD - speedD), speedD, speed, speedLabel);
        x += speedD + gap;
        syncButton.setBounds (x, cy - iconS / 2, iconS, iconS);
        x += iconS + 10;
        rateBox.setBounds (x, cy - boxH / 2, boxW, boxH);
    }

    void syncControls (bool on) override
    {
        const bool sync = shell.param (ID_SYNC) > 0.5f;
        if ((bool) speed.getProperties().getWithDefault ("syncLocked", false) != sync)
        {
            speed.getProperties().set ("syncLocked", sync);
            speed.repaint();
        }
        if ((bool) rateBox.getProperties().getWithDefault ("glowActive", false) != sync)
        {
            rateBox.getProperties().set ("glowActive", sync);
            rateBox.repaint();
        }
        rateBox.setColour (juce::ComboBox::textColourId,
                           ! on ? labelOffColour()
                                : sync ? themePalette().frameRaye.interpolatedWith (juce::Colour (0xfff2f4f8), 0.34f)
                                       : juce::Colour (0xff6a6e78));
    }

    juce::Slider amount;
    SyncLockSlider speed;
    juce::Label amountLabel, speedLabel;
    juce::TextButton pulseButton { "Pulse" }, syncButton { "Sync" };
    juce::ComboBox rateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pulseAttachment, syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;
};

juce::AudioProcessorEditor* AutopanProcessor::createEditor() { return new AutopanEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AutopanProcessor(); }
