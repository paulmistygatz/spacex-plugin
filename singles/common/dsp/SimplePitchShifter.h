#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include "FastMath.h"

// Sehr einfacher, guenstiger "Micro-Pitch"-Shifter (Eventide-MicroPitch-
// Prinzip): klassische Zwei-Tap-Crossfade-Technik mit variabler Verzoegerung
// - zwei um eine halbe Fenstergroesse versetzte Lesekoepfe, deren Delay
// linear ansteigt bzw. abfaellt (das erzeugt den Tonhoehen-Versatz), dabei
// dreieckig ueberblendet. Fuer sehr kleine Verstimmungen (wenige Cent) so
// gut wie artefaktfrei, da das Delay nur extrem langsam driftet (ein voller
// Fenster-Durchlauf dauert bei 10 Cent mehrere Sekunden). Guenstig genug,
// um pro Sample ohne FFT/Buffering-Latenz zu laufen.
class SimplePitchShifter
{
public:
    void prepare (double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        windowSamples = juce::jmax (64, (int) std::round (0.05 * sampleRate)); // ~50ms Fenster
        bufferSize = windowSamples * 2 + 8;
        buffer.assign ((size_t) bufferSize, 0.0f);
        writePos = 0;
        phase = 0.0f;
    }

    // ratio = 2^(cents/1200); 1.0 = keine Verstimmung.
    void setRatio (float newRatio) noexcept { ratio = newRatio; }

    inline float process (float input) noexcept
    {
        if (! std::isfinite (input)) input = 0.0f;
        buffer[(size_t) writePos] = input;

        // Echter Bypass bei (nahezu) 0 Cent: Ohne das wuerde die Phase bei
        // ratio==1 exakt einfrieren (Inkrement = 0) - je nachdem, wo sie
        // einfriert, dominiert dann einer der beiden Lesekoepfe mit einer
        // FESTEN, aber von 0 verschiedenen Verzoegerung (z.B. ein halbes
        // Fenster = ~25ms), statt eines sauberen 1:1-Durchlaufs. Das war der
        // Bug, durch den "Bend" bei 0ct trotzdem hoerbar war (staendiger
        // Kammfilter-/Delay-Effekt). Deshalb hier explizit umgehen.
        if (std::abs (ratio - 1.0f) < 0.0005f)
        {
            writePos = (writePos + 1) % bufferSize;
            return input;
        }

        phase += (1.0f - ratio) / (float) windowSamples;
        if (phase >= 1.0f) phase -= 1.0f;
        if (phase < 0.0f) phase += 1.0f;

       #if SPACEX_CPU_OPT
        const float phaseB = (phase < 0.5f) ? phase + 0.5f : phase - 0.5f;   // = fmod(phase+0.5, 1)
       #else
        const float phaseB = std::fmod (phase + 0.5f, 1.0f);
       #endif

        const float outA = readTap (phase);
        const float outB = readTap (phaseB);

        // Runde 35: Hann- statt Dreiecksfenster (sin^2 + cos^2 = 1, gleiche
        // Lautstaerke, aber ohne Knick an den Uebergaengen -> weicher).
       #if SPACEX_CPU_OPT
        // sin^2(pi*p) = 0.5 - 0.5*cos(2*pi*p), Cosinus aus der Tabelle.
        const float winA = 0.5f - 0.5f * spacex::fastCosCycles ((double) phase);
       #else
        const float sA = std::sin (juce::MathConstants<float>::pi * phase);
        const float winA = sA * sA;
       #endif
        const float winB = 1.0f - winA;

        writePos = (writePos + 1) % bufferSize;

        float out = outA * winA + outB * winB;
        if (! std::isfinite (out)) out = 0.0f;
        return out;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        phase = 0.0f;
    }

private:
    inline float readTap (float p) const noexcept
    {
        const float delaySamples = p * (float) windowSamples;
        float readPosF = (float) writePos - delaySamples;
        while (readPosF < 0.0f) readPosF += (float) bufferSize;
        // Runde 35: kubische (Hermite-)Interpolation statt linear - linear
        // daempft je nach Bruchteil die Hoehen unterschiedlich stark, das
        // "flattert" bei langsam wanderndem Delay. Kostet ein paar
        // Multiplikationen pro Sample, keine Latenz.
        const int r1 = (int) readPosF;
        const float t = readPosF - (float) r1;
        const int r0 = (r1 - 1 + bufferSize) % bufferSize;
        const int r2 = (r1 + 1) % bufferSize;
        const int r3 = (r1 + 2) % bufferSize;
        const float y0 = buffer[(size_t) r0], y1 = buffer[(size_t) r1],
                    y2 = buffer[(size_t) r2], y3 = buffer[(size_t) r3];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    std::vector<float> buffer;
    double sampleRate = 44100.0;
    int windowSamples = 2205;
    int bufferSize = 4418;
    int writePos = 0;
    float phase = 0.0f;
    float ratio = 1.0f;
};
