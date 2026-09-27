#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "CustomLookAndFeel.h"

// ============================================================================
// Runde 182 (SpaceX 1.0.2, User): Frequenz-Ansicht der LCR MATRIX.
//
// Ersetzt auf Wunsch (Umschalter im Sektionskopf) die Regler C-Weight und
// HF Regain durch eine Kurve wie bei einem EQ:
//   - Gold  = was mit C passiert: Delle zwischen Bass Guard und Regain-Grenze,
//             so tief, wie der LCR-Regler steht. Darueber/darunter 0 dB.
//   - Blau  = L/R. Bleibt flach, ausser EQ -> LCR ist an: dann die Form des
//             Sides EQ (nur Anzeige, eingestellt wird er weiter in MID-SIDE).
//   - Darunter ein ruhiger Analyzer aus der LCR-FFT (nur solange sichtbar).
// Bedienung: Flaeche senkrecht = LCR, weisser Griff waagrecht = HF Regain,
// Chip oben links senkrecht = C-Weight. Cmd-Klick oder Doppelklick = Default.
// Alles geht ueber die echten Parameter (mit Gesten), die Regler in der
// anderen Ansicht folgen also automatisch.
// ============================================================================
class LcrGraphComponent : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit LcrGraphComponent (LCRMSAudioProcessor& p) : proc (p)
    {
        smC.fill (-120.0f); smS.fill (-120.0f);
        setWantsKeyboardFocus (false);
    }
    ~LcrGraphComponent() override { setActive (false); }

    // Analyzer + Neuzeichnen nur, solange die Ansicht gezeigt wird.
    void setActive (bool on)
    {
        if (on == active) return;
        active = on;
        proc.getLcrExtractor().setAnalyzerEnabled (on);
        if (on) startTimerHz (30); else stopTimer();
        if (! on) { smC.fill (-120.0f); smS.fill (-120.0f); }
    }

    void paint (juce::Graphics& g) override
    {
        const bool off   = (bool) getProperties().getWithDefault ("sectionOff", false);
        const auto gold  = off ? knobValueOffColour() : themePalette().knob;
        const auto blue  = off ? knobValueOffColour() : themePalette().mod;
        const auto dimTx = juce::Colour (0xff5d6069);
        const auto P     = plotArea();
        auto X = [&] (float f) { return P.getX() + (std::log10 (f) - kL20) / (kL20k - kL20) * P.getWidth(); };
        auto Y = [&] (float db) { return P.getY() + (kTopDb - db) / (kTopDb - kBotDb) * P.getHeight(); };
        auto Ya = [&] (float db) { return P.getBottom() - (db + 66.0f) / 66.0f * P.getHeight() * 0.6f; };

        // Raster + Beschriftung
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        for (float d : { 0.0f, -12.0f, -24.0f })
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRect (P.getX(), Y (d), P.getWidth(), 1.0f);
            g.setColour (dimTx);
            g.drawText (juce::String ((int) d), juce::Rectangle<float> (P.getX() - 24.0f, Y (d) - 6.0f, 20.0f, 12.0f), juce::Justification::centredRight);
        }
        for (float f : { 100.0f, 1000.0f, 10000.0f })
            g.drawText (f >= 1000.0f ? juce::String ((int) (f / 1000.0f)) + "k" : juce::String ((int) f),
                        juce::Rectangle<float> (X (f) - 16.0f, P.getBottom() + 2.0f, 32.0f, 12.0f), juce::Justification::centred);

        const auto st = readState();
        auto cDb = [&] (float f)
        {
            const float lin = juce::jmax (1.0e-4f, 1.0f - st.blend * StereoSTFTExtractor::maskValue (f, st.loHz, st.hiHz, st.sr));
            float d = 20.0f * std::log10 (lin);
            if (st.eqLcr) d += sideeq::responseDb (st.curve, false, f, st.sr);
            return juce::jmax (kBotDb - 3.0f, d);
        };
        auto lrDb = [&] (float f) { return st.eqLcr ? juce::jmax (kBotDb - 3.0f, sideeq::responseDb (st.curve, true, f, st.sr)) : 0.0f; };

        g.saveState();
        g.reduceClipRegion (P.toNearestInt().withTrimmedTop (-6));

        // Analyzer (ruhig, hinter allem)
        if (active && hasAna)
        {
            auto spectrum = [&] (const std::array<float, kB>& arr, std::function<float (float)> post, juce::Colour col, float alpha)
            {
                juce::Path pth; bool started = false;
                for (int i = 0; i < kB; ++i)
                {
                    const float f = StereoSTFTExtractor::anaBandHz (i);
                    if (arr[(size_t) i] <= -119.0f) continue;
                    const float x = X (f), y = Ya (arr[(size_t) i] + post (f));
                    if (! started) { pth.startNewSubPath (x, P.getBottom()); pth.lineTo (x, y); started = true; }
                    else pth.lineTo (x, y);
                }
                if (! started) return;
                pth.lineTo (pth.getCurrentPosition().x, P.getBottom());
                pth.closeSubPath();
                g.setGradientFill (juce::ColourGradient (col.withAlpha (alpha * 0.1f), 0.0f, P.getBottom(),
                                                         col.withAlpha (alpha), 0.0f, P.getBottom() - P.getHeight() * 0.5f, false));
                g.fillPath (pth);
            };
            spectrum (smS, [&] (float f) { return lrDb (f); }, blue, 0.22f);
            spectrum (smC, [&] (float f) { return cDb (f); },  gold, 0.34f);
        }

        auto curvePath = [&] (std::function<float (float)> fn, float xFrom, float xTo)
        {
            juce::Path pth;
            const int n = 200;
            for (int i = 0; i <= n; ++i)
            {
                const float f = std::pow (10.0f, kL20 + (kL20k - kL20) * (float) i / (float) n);
                const float x = X (f);
                if (x < xFrom - 2.0f || x > xTo + 2.0f) continue;
                const float y = Y (fn (f));
                if (pth.isEmpty()) pth.startNewSubPath (x, y); else pth.lineTo (x, y);
            }
            return pth;
        };
        // Blau (L/R)
        g.setColour (blue.withAlpha (st.eqLcr ? 0.95f : 0.35f));
        g.strokePath (curvePath (lrDb, P.getX(), P.getRight()), juce::PathStrokeType (2.0f));
        // Gold (C): unter dem Bass Guard gedimmt, dort bleibt alles unangetastet
        const float xg = st.loHz > 20.5f ? X (st.loHz * 0.98f) : P.getX();
        if (xg > P.getX())
        {
            g.setColour (juce::Colours::white.withAlpha (0.025f));
            g.fillRect (P.getX(), P.getY(), xg - P.getX(), P.getHeight());
            g.setColour (juce::Colour (0x59aaa091));
            g.strokePath (curvePath (cDb, P.getX(), xg), juce::PathStrokeType (1.4f));
        }
        {
            const auto cp = curvePath (cDb, xg, P.getRight());
            g.setColour (gold.withAlpha (0.18f + 0.22f * st.weight));   // Schein = C-Weight
            g.strokePath (cp, juce::PathStrokeType (4.0f + 6.0f * st.weight, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (gold);
            g.strokePath (cp, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.restoreState();

        // Griff HF Regain
        const auto hr = handleRect (st);
        g.setColour (off ? knobValueOffColour() : juce::Colour (0xfff2f4f8));
        g.fillRoundedRectangle (hr, 4.0f);

        // Kopfzeile: C-WEIGHT-Chip, Regain-Frequenz, Sides EQ (nur Anzeige)
        const auto chip = chipRect();
        g.setColour (juce::Colour (0xff1c1e24));
        g.fillRoundedRectangle (chip, chip.getHeight() * 0.5f);
        g.setColour (gold.withAlpha (0.45f));
        g.drawRoundedRectangle (chip.reduced (0.5f), chip.getHeight() * 0.5f, 1.0f);
        g.setColour (gold);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));
        g.drawText ("C-WEIGHT " + juce::String (juce::roundToInt (st.weight * 100.0f)) + " %", chip, juce::Justification::centred);
        g.setColour (off ? dimTx : juce::Colour (0xffe9ebef));
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawText (st.regainOff ? juce::String ("HF Regain Off") : "HF Regain " + hzText (st.hiHz),
                    juce::Rectangle<float> (chip.getRight() + 10.0f, chip.getY(), 130.0f, chip.getHeight()), juce::Justification::centredLeft);
        if (st.eqLcr)
        {
            static const char* names[] = { "TIGHT", "CLEAR", "FOCUS" };
            g.setColour (blue);
            g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));
            g.drawText (juce::String ("SIDES EQ  ") + names[juce::jlimit (0, 2, st.mode)] + "  " + juce::String (juce::roundToInt (st.amt * 100.0f)) + " %",
                        juce::Rectangle<float> (P.getRight() - 170.0f, chip.getY(), 170.0f, chip.getHeight()), juce::Justification::centredRight);
        }
    }

    // --- Maus ----------------------------------------------------------------
    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto m = hitMode (e.position);
        setMouseCursor (m == Mode::regain ? juce::MouseCursor::LeftRightResizeCursor
                                          : juce::MouseCursor::UpDownResizeCursor);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        mode = hitMode (e.position);
        if (mode == Mode::none) return;
        auto* prm = paramFor (mode);
        if (prm == nullptr) { mode = Mode::none; return; }
        if (e.mods.isCommandDown())
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getDefaultValue());
            prm->endChangeGesture();
            mode = Mode::none;
            return;
        }
        startNorm = prm->getValue();
        prm->beginChangeGesture();
        gestureOpen = true;
        if (mode == Mode::regain) dragRegainTo (e.position.x, *prm);
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! gestureOpen) return;
        auto* prm = paramFor (mode);
        if (prm == nullptr) return;
        const float dy = (float) e.getDistanceFromDragStartY();
        const auto P = plotArea();
        if (mode == Mode::regain)      dragRegainTo (e.position.x, *prm);
        else if (mode == Mode::sep)    prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, startNorm + dy / juce::jmax (40.0f, P.getHeight())));
        else if (mode == Mode::weight) prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, startNorm - dy / 200.0f));
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (gestureOpen)
            if (auto* prm = paramFor (mode)) prm->endChangeGesture();
        gestureOpen = false;
        mode = Mode::none;
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (auto* prm = paramFor (hitMode (e.position)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getDefaultValue());
            prm->endChangeGesture();
        }
    }

private:
    static constexpr int   kB     = StereoSTFTExtractor::kAnaBands;
    static constexpr float kL20   = 1.30103f, kL20k = 4.30103f;
    static constexpr float kTopDb = 9.0f, kBotDb = -27.0f;
    enum class Mode { none, sep, weight, regain };

    struct State
    {
        float blend = 0, weight = .5f, loHz = 120, hiHz = 96000, amt = 0; double sr = 48000;
        bool regainOff = true, eqLcr = false; int mode = 3; sideeq::Curve curve {};
    };

    State readState() const
    {
        State s;
        auto& ap = proc.apvts;
        auto raw = [&] (const char* id) { return ap.getRawParameterValue (id)->load(); };
        s.sr     = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0;
        s.blend  = juce::jlimit (0.0f, 1.0f, raw (LCRMSAudioProcessor::ID_LCR_BLEND) * 0.01f);
        s.weight = juce::jlimit (0.0f, 1.0f, raw (LCRMSAudioProcessor::ID_LCR_SENS) * 0.01f);
        s.loHz   = raw (LCRMSAudioProcessor::ID_BASS_GUARD) > 0.5f ? 120.0f : 20.0f;
        const float air = raw (LCRMSAudioProcessor::ID_LCR_HORIZON);
        s.regainOff = air < 0.5f;
        s.hiHz   = s.regainOff ? 96000.0f : 20000.0f * std::pow (0.025f, air * 0.01f);   // wie im Prozessor
        s.mode   = juce::jlimit (0, sideeq::kModes - 1, (int) std::round (raw (LCRMSAudioProcessor::ID_MS_EQ)));
        s.amt    = juce::jlimit (0.0f, 1.0f, raw (LCRMSAudioProcessor::ID_MS_EQ_AMT) * 0.01f);
        s.eqLcr  = raw (LCRMSAudioProcessor::ID_MS_EQ_LCR) > 0.5f && ! sideeq::isFlat (s.mode);
        s.curve  = sideeq::evaluate (s.mode, s.amt, true);
        return s;
    }

    juce::Rectangle<float> plotArea() const
    {
        auto r = getLocalBounds().toFloat();
        r.removeFromTop (28.0f);      // Kopfzeile
        r.removeFromBottom (16.0f);   // Hz-Beschriftung
        r.removeFromLeft (24.0f);     // dB-Beschriftung
        r.removeFromRight (4.0f);
        return r;
    }
    juce::Rectangle<float> chipRect() const { return { plotArea().getX(), 3.0f, 100.0f, 18.0f }; }
    juce::Rectangle<float> handleRect (const State& st) const
    {
        const auto P = plotArea();
        const float f = st.regainOff ? 20000.0f : juce::jlimit (500.0f, 20000.0f, st.hiHz);
        const float x = P.getX() + (std::log10 (f) - kL20) / (kL20k - kL20) * P.getWidth();
        const float lin = juce::jmax (1.0e-4f, 1.0f - st.blend * StereoSTFTExtractor::maskValue (juce::jmin (f, 19000.0f), st.loHz, st.hiHz, st.sr));
        const float db = juce::jmax (kBotDb, 20.0f * std::log10 (lin));
        const float y = P.getY() + (kTopDb - db) / (kTopDb - kBotDb) * P.getHeight();
        return { x - 5.0f, juce::jlimit (P.getY(), P.getBottom() - 22.0f, y - 11.0f), 10.0f, 22.0f };
    }
    Mode hitMode (juce::Point<float> p) const
    {
        if (chipRect().expanded (2.0f).contains (p)) return Mode::weight;
        const auto hr = handleRect (readState());
        if (std::abs (p.x - hr.getCentreX()) < 11.0f && p.y > plotArea().getY() - 4.0f && p.y < plotArea().getBottom() + 4.0f) return Mode::regain;
        if (plotArea().expanded (4.0f).contains (p)) return Mode::sep;
        return Mode::none;
    }
    juce::RangedAudioParameter* paramFor (Mode m) const
    {
        const char* id = m == Mode::sep ? LCRMSAudioProcessor::ID_LCR_BLEND
                       : m == Mode::weight ? LCRMSAudioProcessor::ID_LCR_SENS
                       : m == Mode::regain ? LCRMSAudioProcessor::ID_LCR_HORIZON : nullptr;
        return id != nullptr ? proc.apvts.getParameter (id) : nullptr;
    }
    void dragRegainTo (float x, juce::RangedAudioParameter& prm)
    {
        const auto P = plotArea();
        const float f = std::pow (10.0f, kL20 + (x - P.getX()) / juce::jmax (1.0f, P.getWidth()) * (kL20k - kL20));
        float v = 0.0f;   // ganz rechts = aus
        if (f < 19000.0f)
            v = juce::jlimit (0.0f, 100.0f, std::log (juce::jlimit (500.0f, 20000.0f, f) / 20000.0f) / std::log (0.025f) * 100.0f);
        prm.setValueNotifyingHost (prm.convertTo0to1 (v));
    }
    static juce::String hzText (float f)
    {
        if (f >= 1000.0f) return juce::String (f / 1000.0f, f >= 10000.0f ? 1 : 2) + " kHz";
        return juce::String (juce::roundToInt (f)) + " Hz";
    }

    void timerCallback() override
    {
        auto& ex = proc.getLcrExtractor();
        const int fr = ex.getAnaFrame();
        const bool fresh = fr != lastFrame;
        lastFrame = fr;
        staleTicks = fresh ? 0 : staleTicks + 1;
        bool any = false;
        for (int i = 0; i < kB; ++i)
        {
            float tc = -120.0f, ts = -120.0f;
            if (staleTicks < 8)   // Engine liefert noch (sonst langsam ausblenden)
            {
                const float pc = ex.getAnaCentre (i), ps = ex.getAnaSides (i);
                if (pc >= 0.0f) tc = 10.0f * std::log10 (pc + 1.0e-12f);
                if (ps >= 0.0f) ts = 10.0f * std::log10 (ps + 1.0e-12f);
                if (pc < 0.0f) { smC[(size_t) i] = -120.0f; smS[(size_t) i] = -120.0f; continue; }   // Band ohne Bin
            }
            auto& c = smC[(size_t) i]; auto& s = smS[(size_t) i];
            c += (tc - c) * (tc > c ? 0.30f : 0.07f);   // ruhig: schnell hoch, langsam runter
            s += (ts - s) * (ts > s ? 0.30f : 0.07f);
            any = any || c > -100.0f || s > -100.0f;
        }
        hasAna = any;
        repaint();
    }

    LCRMSAudioProcessor& proc;
    bool active = false, hasAna = false, gestureOpen = false;
    Mode mode = Mode::none;
    float startNorm = 0.0f;
    int lastFrame = 0, staleTicks = 99;   // Frame 0 = Engine hat noch nichts geliefert
    std::array<float, kB> smC {}, smS {};
};

// ============================================================================
// Umschalter im LCR-Kopf: Regler-Icon | Analyzer-Icon. Jeder Klick irgendwo
// darauf wechselt (User: "egal auf welches Icon ich klicke").
// ============================================================================
class LcrViewToggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onToggle;
    void setAnalyzer (bool b) { if (analyzer != b) { analyzer = b; repaint(); } }
    bool isAnalyzer() const { return analyzer; }

    void paint (juce::Graphics& g) override
    {
        const bool off = (bool) getProperties().getWithDefault ("sectionOff", false);
        const auto gold = off ? knobValueOffColour() : themePalette().knob;
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const float w = (r.getWidth() - 3.0f) * 0.5f;
        for (int i = 0; i < 2; ++i)
        {
            const auto b = juce::Rectangle<float> (r.getX() + (float) i * (w + 3.0f), r.getY(), w, r.getHeight());
            const bool on = (i == 1) == analyzer;
            // Runde 183 (User): Sektion aus = dieselbe gedimmte Farbe wie alle Regler.
            if (off)
            {
                g.setColour (juce::Colours::white.withAlpha (on ? 0.03f : 0.0f));
                g.fillRoundedRectangle (b, 5.0f);
                g.setColour (knobRingOffColour());
                g.drawRoundedRectangle (b, 5.0f, 1.0f);
                g.setColour (on ? knobValueOffColour() : knobRingOffColour().brighter (0.15f));
            }
            else
            {
                g.setColour (on ? gold.withAlpha (0.10f) : juce::Colour (0xff1c1e24));
                g.fillRoundedRectangle (b, 5.0f);
                g.setColour (on ? gold.withAlpha (0.45f) : juce::Colours::white.withAlpha (hover ? 0.18f : 0.10f));
                g.drawRoundedRectangle (b, 5.0f, 1.0f);
                g.setColour (on ? gold : juce::Colour (0xff8f96a4));
            }
            const auto c = b.getCentre();
            const float s = juce::jmin (b.getWidth(), b.getHeight()) * 0.30f;
            if (i == 0)
            {   // Regler: Bogen + Zeiger
                juce::Path arc;
                arc.addCentredArc (c.x, c.y + s * 0.1f, s, s, 0.0f, -2.4f, 2.4f, true);
                g.strokePath (arc, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.drawLine (c.x, c.y + s * 0.1f, c.x, c.y - s * 0.6f, 1.5f);
            }
            else
            {   // Analyzer: kleines Spektrum
                juce::Path sp;
                const float ys[] = { 0.7f, 0.2f, 0.4f, -0.5f, -0.1f, -0.8f, -0.3f, -0.45f };
                for (int k = 0; k < 8; ++k)
                {
                    const float x = c.x - s * 1.3f + (float) k * s * 2.6f / 7.0f, y = c.y + ys[k] * s;
                    if (k == 0) sp.startNewSubPath (x, y); else sp.lineTo (x, y);
                }
                g.strokePath (sp, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    }
    void mouseUp (const juce::MouseEvent& e) override { if (e.mouseWasClicked() && onToggle) onToggle(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    bool analyzer = false, hover = false;
};
