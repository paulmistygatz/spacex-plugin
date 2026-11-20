#pragma once
#include "Shell.h"

// Sides-EQ-Bedienung wie in SpaceX' Mid-Side-Sektion: Icon-Feld (Kurve,
// Name, Punkte) und kleiner Staerke-Fader im Kopf. Das Icon gleitet im
// Monitortakt zur neuen Form (SpaceX tickEqIcon).
namespace fairy
{
    class SidesEqUi
    {
    public:
        juce::TextButton field;
        ModeDots dots;
        juce::Slider amount;

        // Im Editor-Konstruktor aufrufen. modeId/amtId: Parameter-IDs.
        template <class Editor>
        void setup (Editor& ed, juce::Component& content, ShellProcessor& p, const char* modeIdIn, const char* amtIdIn,
                    std::function<void (juce::Component&)> addControl,
                    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& amtAttachment)
        {
            proc = &p; modeId = modeIdIn; amtId = amtIdIn;
            field.setClickingTogglesState (false);
            field.setWantsKeyboardFocus (false);
            field.getProperties().set ("thinOnFrame", true);
            field.getProperties().set ("modePill", true);
            field.getProperties().set ("eqDiagram", mode());
            field.getProperties().set ("pillColour", (int) themePalette().frameRaye.getARGB());
            addControl (field);
            field.onClick = [this, &ed]
            {
                constexpr int n = sideeq::kModes;
                const bool back = juce::ModifierKeys::currentModifiers.isCommandDown();
                const int d = (sideeq::displayFromParam (mode()) + (back ? n - 1 : 1)) % n;
                setMode (sideeq::paramFromDisplay (d));
                ed.refreshNow();
            };
            dots.count = sideeq::kModes;
            content.addAndMakeVisible (dots);
            dots.onPick = [this, &ed] (int i) { setMode (sideeq::paramFromDisplay (i)); ed.refreshNow(); };

            amount.setSliderStyle (juce::Slider::LinearHorizontal);
            amount.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            amount.getProperties().set ("miniFader", true);
            amount.getProperties().set ("faderColour", (int) altAccentColour().getARGB());
            amount.setWantsKeyboardFocus (false);
            addControl (amount);
            amtAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, amtId, amount);
            amount.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
        }

        void sync (bool on)
        {
            static const char* const names[sideeq::kModes] = { "FLAT", "TIGHT", "CLEAR", "FOCUS" };
            const int e = mode(), shown = sideeq::displayFromParam (e);
            if (field.getButtonText() != names[shown]) field.setButtonText (names[shown]);
            if ((int) field.getProperties()["eqDiagram"] != e) { field.getProperties().set ("eqDiagram", e); field.repaint(); }
            if (dots.index != shown || dots.off == on) { dots.index = shown; dots.off = ! on; dots.repaint(); }
        }

        // 60 Hz: Icon-Form gleitet zur Zielform (Zeitkonstante ~70 ms).
        void tickIcon()
        {
            const auto L = sideeq::lookFor (mode(), proc->param (amtId) * 0.01f);
            const float tgt[sideeq::kLookFields] = { L.hpX, L.hpSlope, L.hpW, L.sX, L.sG, L.sW, L.mX, L.mG, L.mW };
            bool changed = ! lookInit;
            for (int k = 0; k < sideeq::kLookFields; ++k)
            {
                if (! lookInit) { lookCur[k] = tgt[k]; continue; }
                const float d = tgt[k] - lookCur[k];
                if (d == 0.0f) continue;
                lookCur[k] = std::abs (d) < 1.0e-4f ? tgt[k] : lookCur[k] + d * 0.22f;
                changed = true;
            }
            lookInit = true;
            if (! changed) return;
            juce::Array<juce::var> arr;
            for (int k = 0; k < sideeq::kLookFields; ++k) arr.add ((double) lookCur[k]);
            field.getProperties().set ("eqLook", juce::var (arr));
            field.repaint();
        }

        static void placeFader (juce::Rectangle<int> headerRest, juce::Slider& s)
        {
            const int w = juce::jmin (104, headerRest.getWidth());
            s.setBounds (headerRest.removeFromRight (w).withSizeKeepingCentre (w, 18));
        }

    private:
        int  mode() const { return juce::jlimit (0, sideeq::kModes - 1, (int) std::round (proc->param (modeId))); }
        void setMode (int m)
        {
            if (auto* prm = proc->apvts.getParameter (modeId))
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) m));
        }

        ShellProcessor* proc = nullptr;
        const char* modeId = nullptr;
        const char* amtId = nullptr;
        float lookCur[sideeq::kLookFields] {};
        bool  lookInit = false;
    };
}
