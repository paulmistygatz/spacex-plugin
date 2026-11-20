#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "SideEq.h"   // aus SpaceX (../Source/DSP)

// Der Sides EQ aus SpaceX (Mid-Side und "EQ -> LCR"), 1:1:
// Kurven aus sideeq::evaluate, Koeffizienten gleiten pro Block (40 ms),
// nach dem Umschalten auf FLAT laufen die Filter noch 0,4 s aus.
// Quelle: LCRMSAudioProcessor::updateMsEqCoeffs / processBlock.
namespace fairy
{
    struct Biquad
    {
        struct C { float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; };
        float z1 = 0.0f, z2 = 0.0f;
        float process (float x, const C& c) noexcept
        {
            float y = c.b0 * x + z1;
            z1 = c.b1 * x + z2 - c.a1 * y;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };

    inline void highpassCoeffs (Biquad::C& c, double sampleRate, float freqHz) noexcept
    {
        const double w0 = 2.0 * juce::MathConstants<double>::pi
                          * juce::jlimit (10.0, sampleRate * 0.45, (double) freqHz) / sampleRate;
        const double cosW = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * 0.70710678);
        const double a0 = 1.0 + alpha;
        c.b0 = (float) (((1.0 + cosW) * 0.5) / a0);
        c.b1 = (float) ((-(1.0 + cosW)) / a0);
        c.b2 = c.b0;
        c.a1 = (float) ((-2.0 * cosW) / a0);
        c.a2 = (float) ((1.0 - alpha) / a0);
    }

    class SidesEqCoeffs
    {
    public:
        Biquad::C s1, s2, sHs, mHs;   // Seite: Hochpass 1, Hochpass 2, High Shelf; Mitte: High Shelf

        void prepare (double sampleRate, int mode, float amount01)
        {
            sr = sampleRate > 0.0 ? sampleRate : 48000.0;
            holdSamples = 0;
            update (mode, amount01, true, 0);
        }

        // Pro Block. Gibt zurueck, ob der EQ in diesem Block laufen muss.
        bool beginBlock (int mode, float amount01, int numSamples)
        {
            if (! sideeq::isFlat (mode))
                holdSamples = (int) (sr * 0.4);
            else
                holdSamples = juce::jmax (0, holdSamples - numSamples);
            const bool active = holdSamples > 0;
            if (active)
                update (mode, amount01, false, numSamples);
            return active;
        }

    private:
        void update (int mode, float amount01, bool snap, int numSamples)
        {
            const auto t = sideeq::evaluate (juce::jlimit (0, sideeq::kModes - 1, mode), amount01, true);
            const float tgt[10] = { std::log2 (t.s1Hz), t.s1Q, std::log2 (t.s2Hz), t.s2Q,
                                    std::log2 (t.sHsHz), t.sHsDb, t.sHsQ,
                                    std::log2 (t.mHsHz), t.mHsDb, t.mHsQ };
            const float a = snap ? 0.0f : std::exp (-(float) numSamples / (0.040f * (float) sr));
            for (int k = 0; k < 10; ++k)
                cur[k] += (1.0f - a) * (tgt[k] - cur[k]);
            auto toBq = [] (Biquad::C& d, const sideeq::Coeffs& c)
            {
                d.b0 = (float) c.b0; d.b1 = (float) c.b1; d.b2 = (float) c.b2;
                d.a1 = (float) c.a1; d.a2 = (float) c.a2;
            };
            toBq (s1,  sideeq::highpass  (sr, std::exp2 (cur[0]), cur[1]));
            toBq (s2,  sideeq::highpass  (sr, std::exp2 (cur[2]), cur[3]));
            toBq (sHs, sideeq::highShelf (sr, std::exp2 (cur[4]), cur[5], cur[6]));
            toBq (mHs, sideeq::highShelf (sr, std::exp2 (cur[7]), cur[8], cur[9]));
        }

        double sr = 48000.0;
        float  cur[10] {};
        int    holdSamples = 0;
    };

    // Seitenkette: Hochpass 1 -> Hochpass 2 -> High Shelf.
    struct SideChainEq
    {
        Biquad h1, h2, hs;
        float process (float x, const SidesEqCoeffs& c) noexcept { return hs.process (h2.process (h1.process (x, c.s1), c.s2), c.sHs); }
        void clear() noexcept { h1 = {}; h2 = {}; hs = {}; }
    };
}
