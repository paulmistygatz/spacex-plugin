#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "ChannelDelayLine.h"
#include "SimplePitchShifter.h"

// Micropitch aus SpaceX (Sektion "Micropitch", Build SPACEX_PARALLAX_UI=2),
// 1:1 uebernommen: Modus + Amount -> Wegpunkt (Drift %, Shift ct, Tilt %,
// Mix %, Pegel dB, Balance %), Haas-Delay + Mini-Detune, Balance-Ausgleich,
// Tilt, eigener Mix gegen das Original, 120-Hz-Bassschutz auf der
// Differenz, Pegel und Balance des Modus, weiches Ein/Aus.
// Werte und Rechenweg stammen aus PluginProcessor.cpp (SpaceX 1.0.2) -
// wer hier etwas aendert, aendert den Klang.
namespace micropitch
{
    struct Point { float driftPct, bendCt, tiltPct, mixPct, gainDb, panPct; };
    struct ModeDef { int numPoints; bool amountIsMix; bool balance; float amountMax; Point pts[4]; };

    constexpr int kModes = 4;

    inline const ModeDef& modeDef (int mode) noexcept
    {
        static const ModeDef defs[kModes] = {
            // VELVET
            { 1, true,  false, 0.6490f, { {  -19.3f, 0.76f,  8.0f, 39.8f, 2.01f, 39.8f } } },
            // HALO
            { 1, true,  false, 1.0000f, { {  -90.3f, 0.09f, 27.5f, 38.0f, 0.17f, 22.6f } } },
            // ILLUSION
            { 2, false, true,  0.8125f, { {  -38.6f, 6.58f,  0.0f, 30.7f, 0.00f, -3.0f },
                                          { -100.0f, 6.39f,  6.9f, 32.9f, 0.00f, -3.0f } } },
            // DOUBLE
            { 1, true,  true,  1.0000f, { { -100.0f, 0.00f, 27.5f, 36.7f, 0.00f, 0.0f } } }
        };
        return defs[juce::jlimit (0, kModes - 1, mode)];
    }

    inline Point evalMode (int mode, float amount01) noexcept
    {
        const auto& d = modeDef (mode);
        amount01 = juce::jlimit (0.0f, 1.0f, amount01) * d.amountMax;
        if (d.amountIsMix)
        {
            Point p = d.pts[0];
            p.mixPct = d.pts[0].mixPct * amount01;
            p.gainDb = d.pts[0].gainDb * amount01;
            p.panPct = d.pts[0].panPct * amount01;
            return p;
        }
        const Point zero { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        const int segs = d.numPoints;
        const float pos = amount01 * (float) segs;
        const int   i0  = juce::jlimit (0, segs - 1, (int) std::floor (pos));
        const float t   = pos - (float) i0;
        const auto& a = (i0 == 0) ? zero : d.pts[i0 - 1];
        const auto& b = d.pts[i0];
        auto lerp = [t] (float x, float y) { return x + (y - x) * t; };
        return { lerp (a.driftPct, b.driftPct), lerp (a.bendCt, b.bendCt), lerp (a.tiltPct, b.tiltPct),
                 lerp (a.mixPct, b.mixPct), lerp (a.gainDb, b.gainDb), lerp (a.panPct, b.panPct) };
    }

    inline float driftPercentToMs (float absPercent) noexcept
    {
        absPercent = juce::jlimit (0.0f, 100.0f, absPercent);
        if (absPercent <= 80.0f)
            return (absPercent / 80.0f) * 2.0f;
        return 2.0f + ((absPercent - 80.0f) / 20.0f) * 18.0f;
    }

    class Engine
    {
    public:
        void prepare (double sampleRate)
        {
            sr = sampleRate;
            delayL.prepare (sampleRate, 25.0f);
            delayR.prepare (sampleRate, 25.0f);
            bendL.prepare (sampleRate);
            bendR.prepare (sampleRate);
            lastBendForRatio = -1.0e9f;

            setHighpass (hpC, sampleRate, 120.0f);
            hpL = {}; hpR = {};

            constexpr double knobRampSeconds = 0.02;
            for (auto* sv : { &bendS, &mixS, &gainS, &tiltLS, &tiltRS, &panLS, &panRS })
                sv->reset (sampleRate, knobRampSeconds);
            bendS.setCurrentAndTargetValue (0.0f);
            mixS.setCurrentAndTargetValue (0.0f);
            gainS.setCurrentAndTargetValue (1.0f);
            panLS.setCurrentAndTargetValue (1.0f);
            panRS.setCurrentAndTargetValue (1.0f);
            tiltLS.setCurrentAndTargetValue (1.0f);
            tiltRS.setCurrentAndTargetValue (1.0f);

            balanceOn.reset (sampleRate, 0.03);
            balanceOn.setCurrentAndTargetValue (0.0f);
            onGain.reset (sampleRate, 0.03);
            onGain.setCurrentAndTargetValue (1.0f);
        }

        void reset()
        {
            delayL.reset(); delayR.reset();
            bendL.reset();  bendR.reset();
            hpL = {}; hpR = {};
        }

        // Einmal pro Block, vor process().
        void setParameters (bool on, int mode, float amount01) noexcept
        {
            const auto pt = evalMode (mode, amount01);

            bendS.setTargetValue (pt.bendCt);
            mixS.setTargetValue (juce::jlimit (0.0f, 1.0f, pt.mixPct * 0.01f));
            gainS.setTargetValue (juce::Decibels::decibelsToGain (pt.gainDb));
            {
                const float pn = juce::jlimit (-1.0f, 1.0f, pt.panPct * 0.01f);
                panLS.setTargetValue (pn > 0.0f ? 1.0f - pn : 1.0f);
                panRS.setTargetValue (pn < 0.0f ? 1.0f + pn : 1.0f);
            }
            {
                const float panPos = juce::jlimit (-1.0f, 1.0f, pt.tiltPct * 0.01f);
                const float angle  = (panPos + 1.0f) * (juce::MathConstants<float>::pi * 0.25f);
                tiltLS.setTargetValue (std::cos (angle) * juce::MathConstants<float>::sqrt2);
                tiltRS.setTargetValue (std::sin (angle) * juce::MathConstants<float>::sqrt2);
            }

            const float driftPct = pt.driftPct;
            const float driftMs  = driftPercentToMs (std::abs (driftPct));
            delayL.setDelayMs (driftPct >= 0.0f ? driftMs : 0.0f);
            delayR.setDelayMs (driftPct <  0.0f ? driftMs : 0.0f);

            balanceOn.setTargetValue (modeDef (mode).balance ? 1.0f : 0.0f);
            constexpr float kBalanceDbPerMs = 2.75f;
            constexpr float kBalanceMaxMs   = 2.0f;
            balanceCompGain   = juce::Decibels::decibelsToGain (juce::jmin (driftMs, kBalanceMaxMs) * kBalanceDbPerMs);
            balanceBoostsLeft = driftPct >= 0.0f;

            onGain.setTargetValue (on ? 1.0f : 0.0f);
        }

        void process (float* l, float* r, int numSamples) noexcept
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float L = l[i], R = r[i];
                const float bend  = bendS.getNextValue();
                const float dGain = onGain.getNextValue();

                if (bend != lastBendForRatio)
                {
                    lastBendForRatio = bend;
                    bendL.setRatio (std::exp2 (-bend / 1200.0f));
                    bendR.setRatio (std::exp2 ( bend / 1200.0f));
                }

                const float lDrift = bendL.process (delayL.process (L));
                const float rDrift = bendR.process (delayR.process (R));

                const float balanceAmt  = balanceOn.getNextValue();
                const float balanceGain = 1.0f + (balanceCompGain - 1.0f) * balanceAmt;
                const float lDriftBal = balanceBoostsLeft ? lDrift * balanceGain : lDrift;
                const float rDriftBal = balanceBoostsLeft ? rDrift : rDrift * balanceGain;

                const float pxM  = mixS.getNextValue();
                const float pxG  = gainS.getNextValue();
                const float pxTL = tiltLS.getNextValue();
                const float pxTR = tiltRS.getNextValue();
                // Gefiltert wird nur die Differenz zum Original: unter 120 Hz
                // bleibt nass = trocken, der Bass wird nicht verbreitert.
                const float diffL = hpL.process (lDriftBal * pxTL - L, hpC);
                const float diffR = hpR.process (rDriftBal * pxTR - R, hpC);
                const float outL = (L + diffL * pxM) * pxG * panLS.getNextValue();
                const float outR = (R + diffR * pxM) * pxG * panRS.getNextValue();
                l[i] = L + (outL - L) * dGain;
                r[i] = R + (outR - R) * dGain;
            }
        }

    private:
        struct BiquadCoeffs { float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; };
        struct BiquadState
        {
            float z1 = 0.0f, z2 = 0.0f;
            float process (float x, const BiquadCoeffs& c) noexcept
            {
                float y = c.b0 * x + z1;
                z1 = c.b1 * x + z2 - c.a1 * y;
                z2 = c.b2 * x - c.a2 * y;
                return y;
            }
        };
        static void setHighpass (BiquadCoeffs& c, double sampleRate, float freqHz) noexcept
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

        double sr = 44100.0;
        ChannelDelayLine delayL, delayR;
        SimplePitchShifter bendL, bendR;
        float lastBendForRatio = -1.0e9f;
        BiquadCoeffs hpC;
        BiquadState  hpL, hpR;
        juce::SmoothedValue<float> bendS, mixS, gainS, tiltLS, tiltRS, panLS, panRS, balanceOn, onGain;
        float balanceCompGain = 1.0f;
        bool  balanceBoostsLeft = true;
    };
}
