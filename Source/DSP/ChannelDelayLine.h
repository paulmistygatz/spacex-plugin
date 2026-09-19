#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>
#include <algorithm>

// Einfache fraktionale Delay-Line (lineare Interpolation) pro Kanal.
// Wird fuer den unabhaengigen Drift-Parameter (Haas-Effekt) verwendet.
// Dieser Delay ist ein bewusster klanglicher Effekt (Phasenausrichtung),
// KEINE Host-Latenz -> wird nicht ueber setLatencySamples() gemeldet.
//
// Die Zielzeit wird sanft nachgefuehrt (kein harter Sprung), damit
// schnelles Drehen des Reglers keine Knackser/Zipper-Noise erzeugt.
// Bei (nahezu) 0ms wird die Interpolation komplett umgangen und das
// Signal 1:1 durchgereicht - vermeidet jegliche Altdaten-Artefakte im
// Ringpuffer bei der Default-Position.
class ChannelDelayLine
{
public:
    void prepare (double sampleRateIn, float maxDelayMs)
    {
        sampleRate = sampleRateIn;
        maxSamples = std::max (8, (int) std::ceil (maxDelayMs * 0.001 * sampleRate) + 4);
        buffer.assign ((size_t) maxSamples, 0.0f);
        writePos = 0;
        currentDelaySamples = 0.0f;
        targetDelaySamples = 0.0f;

        // Rampzeit fuer Delaenderungen, verhindert Zipper-Noise.
        const float rampSeconds = 0.02f;
        smoothingCoeff = std::exp (-1.0f / (rampSeconds * (float) sampleRate));
    }

    void setDelayMs (float ms)
    {
        float samples = (float) (ms * 0.001 * sampleRate);
        targetDelaySamples = juce::jlimit (0.0f, (float) (maxSamples - 2), samples);
    }

    inline float process (float input) noexcept
    {
        // Sanitize: verhindert, dass ein einmal entstandener NaN/Inf-Wert
        // dauerhaft im Ringpuffer zirkuliert und Knistern verursacht.
        if (! std::isfinite (input))
            input = 0.0f;

        buffer[(size_t) writePos] = input;

        // Zielwert langsam nachfuehren (Ein-Pol-Glaettung).
        currentDelaySamples = targetDelaySamples + smoothingCoeff * (currentDelaySamples - targetDelaySamples);

        float out;
        if (currentDelaySamples < 1.0e-4f)
        {
            // Praktisch kein Delay: direkt durchreichen, keine Interpolation
            // noetig (vermeidet jegliche Altdaten-Artefakte im Puffer).
            out = input;
        }
        else
        {
            float readPosF = (float) writePos - currentDelaySamples;
            while (readPosF < 0.0f)
                readPosF += (float) maxSamples;

            int readPos0 = (int) readPosF;
            int readPos1 = (readPos0 + 1) % maxSamples;
            float frac = readPosF - (float) readPos0;

            out = buffer[(size_t) readPos0] * (1.0f - frac) + buffer[(size_t) readPos1] * frac;
        }

        writePos = (writePos + 1) % maxSamples;

        if (! std::isfinite (out))
            out = 0.0f;

        return out;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        currentDelaySamples = 0.0f;
        targetDelaySamples = 0.0f;
    }

private:
    std::vector<float> buffer;
    double sampleRate = 44100.0;
    int maxSamples = 0;
    int writePos = 0;
    float currentDelaySamples = 0.0f;
    float targetDelaySamples = 0.0f;
    float smoothingCoeff = 0.0f;
};
