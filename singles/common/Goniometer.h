#pragma once
#include "FairyLook.h"
#include "MeterFifo.h"
#include <array>

namespace fairy
{
    // Schlichtes Goniometer (Punktwolke) plus Korrelationsbalken.
    // Oben = Mitte (L+R), links/rechts = Seiten. Die Wolke wird automatisch
    // auf eine gut sichtbare Groesse gebracht (langsame Pegelnachfuehrung),
    // damit auch leise Spuren etwas zeigen.
    class Goniometer : public juce::Component, private juce::Timer
    {
    public:
        explicit Goniometer (mistycat::MeterFifo& f) : fifo (f)
        {
            setInterceptsMouseClicks (false, false);
            tmpL.resize (kMaxPull);
            tmpR.resize (kMaxPull);
            for (auto& f : frames) f.reserve (1500);
        }
        ~Goniometer() override { stopTimer(); }

        void setActive (bool a)
        {
            if (a == active) return;
            active = a;
            if (a) { fifo.clear(); startTimerHz (30); }
            else   stopTimer();
        }

        void resized() override
        {
            scope = getLocalBounds().withTrimmedBottom (kCorrH).reduced (2);
        }

        void paint (juce::Graphics& g) override
        {
            const auto sf = scope.toFloat();
            g.setColour (juce::Colour (0xff0a0b16).withAlpha (0.78f));
            g.fillRoundedRectangle (sf, 8.0f);

            // Achsen: Mitte senkrecht, Seiten waagerecht, L und R diagonal.
            const auto c = sf.getCentre();
            const float R = juce::jmin (sf.getWidth(), sf.getHeight()) * 0.44f;
            g.setColour (frameMain.withAlpha (0.14f));
            g.drawLine (c.x, c.y - R, c.x, c.y + R, 1.0f);
            g.drawLine (c.x - R, c.y, c.x + R, c.y, 1.0f);
            const float d = R * 0.7071f;
            g.setColour (frameMain.withAlpha (0.09f));
            g.drawLine (c.x - d, c.y - d, c.x + d, c.y + d, 1.0f);
            g.drawLine (c.x + d, c.y - d, c.x - d, c.y + d, 1.0f);

            // Punkte der letzten Bilder, aeltere blasser (Nachleuchten).
            {
                const float ox = sf.getX(), oy = sf.getY();
                static const float alphas[kFrames] = { 0.60f, 0.34f, 0.18f, 0.09f };
                for (int k = kFrames - 1; k >= 0; --k)
                {
                    const auto& pts = frames[(size_t) ((head - k + kFrames) % kFrames)];
                    if (pts.empty()) continue;
                    g.setColour (knob.withAlpha (alphas[k]));
                    for (auto& p : pts)
                        g.fillRect (ox + p.x - 0.6f, oy + p.y - 0.6f, 1.2f, 1.2f);
                }
            }

            g.setColour (labelOn.withAlpha (0.45f));
            g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)).withExtraKerningFactor (0.08f));
            g.drawText ("L", juce::Rectangle<float> (sf.getX() + 9.0f, sf.getY() + 6.0f, 12.0f, 12.0f), juce::Justification::centredLeft);
            g.drawText ("R", juce::Rectangle<float> (sf.getRight() - 21.0f, sf.getY() + 6.0f, 12.0f, 12.0f), juce::Justification::centredRight);

            // Korrelation: -1 links, +1 rechts.
            auto bar = juce::Rectangle<float> (sf.getX() + 22.0f, (float) getHeight() - (float) kCorrH * 0.5f - 2.0f,
                                               sf.getWidth() - 44.0f, 4.0f);
            g.setColour (juce::Colour (0xff2a2e4a));
            g.fillRoundedRectangle (bar, 2.0f);
            g.setColour (frameMain.withAlpha (0.30f));
            g.fillRect (bar.getCentreX() - 0.5f, bar.getY() - 1.0f, 1.0f, bar.getHeight() + 2.0f);
            g.setColour (labelOn.withAlpha (0.40f));
            g.setFont (juce::Font (juce::FontOptions (10.0f)));
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x88\x92" "1"), juce::Rectangle<float> (sf.getX(), bar.getCentreY() - 6.0f, 20.0f, 12.0f), juce::Justification::centredRight);
            g.drawText ("+1", juce::Rectangle<float> (bar.getRight() + 3.0f, bar.getCentreY() - 6.0f, 20.0f, 12.0f), juce::Justification::centredLeft);
            if (corrAlpha > 0.01f)
            {
                const float x = bar.getX() + (corr * 0.5f + 0.5f) * bar.getWidth();
                softGlow (g, { x, bar.getCentreY() }, 8.0f, knob, corrAlpha);
                g.setColour (knob.withAlpha (corrAlpha));
                g.fillRoundedRectangle (x - 1.5f, bar.getY() - 2.0f, 3.0f, bar.getHeight() + 4.0f, 1.5f);
            }
        }

    private:
        static constexpr int kCorrH = 26;
        static constexpr int kMaxPull = 8192;
        static constexpr int kFrames = 4;

        void timerCallback() override
        {
            const int n = fifo.pull (tmpL.data(), tmpR.data(), kMaxPull);
            float peak = 0.0f;
            double sLR = 0.0, sLL = 0.0, sRR = 0.0;
            for (int i = 0; i < n; ++i)
            {
                peak = juce::jmax (peak, std::abs (tmpL[(size_t) i]), std::abs (tmpR[(size_t) i]));
                sLR += (double) tmpL[(size_t) i] * tmpR[(size_t) i];
                sLL += (double) tmpL[(size_t) i] * tmpL[(size_t) i];
                sRR += (double) tmpR[(size_t) i] * tmpR[(size_t) i];
            }
            // Pegelnachfuehrung: schnell rauf, langsam runter, nie mehr als +40 dB.
            env = peak > env ? peak : env * 0.96f + peak * 0.04f;
            const float gain = 0.92f / juce::jmax (env, 0.01f);

            head = (head + 1) % kFrames;
            auto& pts = frames[(size_t) head];
            pts.clear();
            const float w = (float) scope.getWidth(), h = (float) scope.getHeight();
            const float cx = w * 0.5f, cy = h * 0.5f, R = juce::jmin (w, h) * 0.44f;
            const int step = juce::jmax (1, n / 1400);
            for (int i = 0; i < n; i += step)
            {
                const float L = tmpL[(size_t) i] * gain, Rr = tmpR[(size_t) i] * gain;
                float px = (Rr - L) * 0.7071f, py = (L + Rr) * 0.7071f;
                const float m = std::sqrt (px * px + py * py);
                if (m > 1.0f) { px /= m; py /= m; }   // sanft am Rand halten
                pts.push_back ({ cx + px * R, cy - py * R });
            }

            const double den = std::sqrt (sLL * sRR);
            if (den > 1.0e-9)
            {
                const float cNow = (float) juce::jlimit (-1.0, 1.0, sLR / den);
                corr = corr * 0.80f + cNow * 0.20f;
                corrAlpha = juce::jmin (1.0f, corrAlpha + 0.15f);
            }
            else
                corrAlpha = juce::jmax (0.0f, corrAlpha - 0.05f);

            repaint();
        }

        mistycat::MeterFifo& fifo;
        std::vector<float> tmpL, tmpR;
        std::array<std::vector<juce::Point<float>>, kFrames> frames;
        int head = 0;
        juce::Rectangle<int> scope;
        bool  active = false;
        float env = 0.0f, corr = 1.0f, corrAlpha = 0.0f;
    };
}
