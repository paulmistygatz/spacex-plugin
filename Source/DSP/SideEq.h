#pragma once
#include <cmath>
#include <algorithm>

// ===== SEITEN-EQ (Runde 125) =====
// Pauls Pro-Q-Kurven, gemeinsam fuer Audio (PluginProcessor) und Icon
// (CustomLookAndFeel) - das Icon zeichnet damit exakt die Kurve, die klingt.
//
// Pro Modus zwei Kurven: A (Fader 50 %) und B (Fader 100 %). Fader 0 % ist
// "nichts". Dazwischen wird nicht das Audio, sondern werden die Filterwerte
// ueberblendet (Frequenz in Oktaven, Gain und Q linear).
//
// Aufbau: Side = Hochpass (zwei 2-pol-Stufen, 12 oder 24 dB/Okt) + High
// Shelf; Mid = High Shelf. Eine 12-dB-Kurve parkt die zweite Stufe bei 10 Hz.
namespace sideeq
{
    constexpr int kModes = 3;   // 0 TIGHT, 1 CLEAR, 2 FOCUS

    struct Curve
    {
        float s1Hz, s1Q, s2Hz, s2Q;      // Side-Hochpass, Stufe 1 und 2
        float sHsHz, sHsDb, sHsQ;        // Side High Shelf
        float mHsHz, mHsDb, mHsQ;        // Mid High Shelf
    };

    // 24 dB/Okt aus zwei Stufen: Butterworth-Verteilung, mit dem Pro-Q-Q skaliert.
    constexpr float kQ24a = 0.765f, kQ24b = 1.848f;
    constexpr float kOffHz = 10.0f, kOffQ = 0.707f;

    inline Curve curveA (int mode) noexcept
    {
        switch (mode)
        {
            case 1:  return { 350.0f, 0.9f, kOffHz, kOffQ,  3000.0f,  3.0f, 0.5f,  6000.0f, 0.0f, 0.3f };   // CLEAR
            case 2:  return { 350.0f, 0.9f, kOffHz, kOffQ,  8000.0f, -8.0f, 0.4f,  6000.0f, 2.0f, 0.3f };   // FOCUS
            default: return { 350.0f, 0.9f, kOffHz, kOffQ,  3000.0f,  0.0f, 0.5f,  6000.0f, 0.0f, 0.3f };   // TIGHT
        }
    }

    inline Curve curveB (int mode) noexcept
    {
        switch (mode)
        {
            case 1:  return { 500.0f, 0.6f, kOffHz, kOffQ, 12000.0f,   5.0f, 0.7f, 6000.0f, 0.0f, 0.3f };  // CLEAR
            case 2:  return { 600.0f, 0.8f * kQ24a, 600.0f, 0.8f * kQ24b,
                                                     8000.0f, -20.0f, 0.8f, 6000.0f, 3.0f, 0.3f };          // FOCUS (24 dB/Okt)
            default: return { 500.0f, 0.6f, kOffHz, kOffQ,  3000.0f,   0.0f, 0.5f, 6000.0f, 0.0f, 0.3f };  // TIGHT
        }
    }

    // "Nichts": Hochpaesse ganz unten, Shelves auf 0 dB (Frequenz/Q wie A,
    // damit beim Einblenden nichts wandert).
    inline Curve curveOff (int mode) noexcept
    {
        Curve c = curveA (mode);
        c.s1Hz = kOffHz; c.s1Q = kOffQ; c.s2Hz = kOffHz; c.s2Q = kOffQ;
        c.sHsDb = 0.0f; c.mHsDb = 0.0f;
        return c;
    }

    inline float lerpLog (float a, float b, float t) noexcept
    {
        return std::exp2 (std::log2 (a) + (std::log2 (b) - std::log2 (a)) * t);
    }
    inline float lerpLin (float a, float b, float t) noexcept { return a + (b - a) * t; }

    inline Curve mix (const Curve& a, const Curve& b, float t) noexcept
    {
        return { lerpLog (a.s1Hz, b.s1Hz, t), lerpLin (a.s1Q, b.s1Q, t),
                 lerpLog (a.s2Hz, b.s2Hz, t), lerpLin (a.s2Q, b.s2Q, t),
                 lerpLog (a.sHsHz, b.sHsHz, t), lerpLin (a.sHsDb, b.sHsDb, t), lerpLin (a.sHsQ, b.sHsQ, t),
                 lerpLog (a.mHsHz, b.mHsHz, t), lerpLin (a.mHsDb, b.mHsDb, t), lerpLin (a.mHsQ, b.mHsQ, t) };
    }

    // amount01: 0 = nichts, 0.5 = A, 1 = B.
    inline Curve evaluate (int mode, float amount01, bool on) noexcept
    {
        mode = std::clamp (mode, 0, kModes - 1);
        if (! on) return curveOff (mode);
        const float a = std::clamp (amount01, 0.0f, 1.0f);
        if (a <= 0.5f) return mix (curveOff (mode), curveA (mode), a * 2.0f);
        return mix (curveA (mode), curveB (mode), (a - 0.5f) * 2.0f);
    }

    // ---- Biquads (RBJ), normiert auf a0 = 1 ----
    struct Coeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

    inline Coeffs highpass (double sr, double hz, double q) noexcept
    {
        hz = std::clamp (hz, 10.0, sr * 0.45);
        q  = std::max (0.1, q);
        const double w0 = 2.0 * 3.14159265358979323846 * hz / sr;
        const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + alpha;
        return { (1.0 + cw) * 0.5 / a0, -(1.0 + cw) / a0, (1.0 + cw) * 0.5 / a0,
                 -2.0 * cw / a0, (1.0 - alpha) / a0 };
    }

    inline Coeffs highShelf (double sr, double hz, double db, double q) noexcept
    {
        hz = std::clamp (hz, 20.0, sr * 0.45);
        q  = std::max (0.1, q);
        const double A  = std::pow (10.0, db / 40.0);
        const double w0 = 2.0 * 3.14159265358979323846 * hz / sr;
        const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
        const double sA2 = 2.0 * std::sqrt (A) * alpha;
        const double b0 =        A * ((A + 1.0) + (A - 1.0) * cw + sA2);
        const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw);
        const double b2 =        A * ((A + 1.0) + (A - 1.0) * cw - sA2);
        const double a0 =             (A + 1.0) - (A - 1.0) * cw + sA2;
        const double a1 =  2.0 *     ((A - 1.0) - (A + 1.0) * cw);
        const double a2 =             (A + 1.0) - (A - 1.0) * cw - sA2;
        return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
    }

    inline double magDb (const Coeffs& c, double sr, double hz) noexcept
    {
        const double w = 2.0 * 3.14159265358979323846 * hz / sr;
        const double c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2.0 * w), s2 = std::sin (2.0 * w);
        const double nr = c.b0 + c.b1 * c1 + c.b2 * c2, ni = -(c.b1 * s1 + c.b2 * s2);
        const double dr = 1.0 + c.a1 * c1 + c.a2 * c2,  di = -(c.a1 * s1 + c.a2 * s2);
        const double num = nr * nr + ni * ni, den = std::max (1.0e-30, dr * dr + di * di);
        return 10.0 * std::log10 (std::max (1.0e-30, num / den));
    }

    // Frequenzgang fuer das Icon: Side (Hochpass + Shelf) oder Mid (Shelf).
    inline float responseDb (const Curve& c, bool side, double hz, double sr = 48000.0) noexcept
    {
        if (! side)
            return (float) magDb (highShelf (sr, c.mHsHz, c.mHsDb, c.mHsQ), sr, hz);
        return (float) (magDb (highpass (sr, c.s1Hz, c.s1Q), sr, hz)
                      + magDb (highpass (sr, c.s2Hz, c.s2Q), sr, hz)
                      + magDb (highShelf (sr, c.sHsHz, c.sHsDb, c.sHsQ), sr, hz));
    }

    // ===== ICON (Runde 126) =====
    // Nicht die exakte Kurve, sondern eine gezeichnete Form, die aussieht wie
    // in Pro-Q und fliessend morpht: Hochpass als gerade Linie nach unten
    // mit weichem Knie (Softplus), Shelves als weiche S-Kurve (Sigmoid).
    // Alle Werte sind stetig - das Icon kann zwischen Modi und Faderstellungen
    // ueberblenden, ohne Ecken und ohne Spruenge.
    // x: 0..1 = 20 Hz..20 kHz (log), y: +1 oben, 0 = 0 dB.
    struct Look { float hpX, hpSlope, hpW, sX, sG, sW, mX, mG, mW; };
    constexpr int kLookFields = 9;

    inline float xOf (float hz) noexcept { return std::log10 (std::max (hz, 20.0f) / 20.0f) / 3.0f; }

    inline Look lookOf (const Curve& c) noexcept
    {
        const bool  o24 = c.s2Hz > 20.0f;
        const float q   = o24 ? c.s1Q / kQ24a : c.s1Q;
        return { xOf (c.s1Hz), o24 ? 7.0f : 4.0f, 0.032f / std::max (q, 0.3f),
                 xOf (c.sHsHz), c.sHsDb, 0.045f / std::max (c.sHsQ, 0.2f),
                 xOf (c.mHsHz), c.mHsDb, 0.045f / std::max (c.mHsQ, 0.2f) };
    }

    // Unter 50 % bleibt die Form des Modus erkennbar (mindestens 35 %
    // Auspraegung) - das Icon zeigt WAS der Modus macht, der Fader WIE VIEL.
    inline Look lookFor (int mode, float amount01) noexcept
    {
        mode = std::clamp (mode, 0, kModes - 1);
        const Look A = lookOf (curveA (mode)), B = lookOf (curveB (mode));
        const float a = std::clamp (amount01, 0.0f, 1.0f);
        if (a <= 0.5f)
        {
            const float k = 0.35f + 0.65f * (a * 2.0f);
            Look L = A;
            L.hpSlope *= k; L.sG *= k; L.mG *= k;
            return L;
        }
        const float t = (a - 0.5f) * 2.0f;
        auto m = [t] (float p, float q) { return p + (q - p) * t; };
        return { m (A.hpX, B.hpX), m (A.hpSlope, B.hpSlope), m (A.hpW, B.hpW),
                 m (A.sX, B.sX), m (A.sG, B.sG), m (A.sW, B.sW),
                 m (A.mX, B.mX), m (A.mG, B.mG), m (A.mW, B.mW) };
    }

    inline float lookY (const Look& L, bool side, float x) noexcept
    {
        auto sig = [] (float z) { return 1.0f / (1.0f + std::exp (-z)); };
        if (! side)
            return 0.9f * std::tanh (L.mG * sig ((x - L.mX) / L.mW) / 8.0f);
        const float shelf = 0.9f * std::tanh (L.sG * sig ((x - L.sX) / L.sW) / 8.0f);
        const float d  = (L.hpX - x) / L.hpW;
        const float sp = L.hpW * (d > 20.0f ? d : std::log1p (std::exp (d)));
        return shelf - L.hpSlope * sp;
    }
}
