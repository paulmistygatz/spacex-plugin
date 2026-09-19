#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <memory>

// Stereo-zu-LCR Extraktion nach dem Azimuth-/Korrelations-Prinzip
// (aehnliches Grundprinzip wie Bertom Phantom Center):
//
// Pro Frequenzbin wird bestimmt, wie "zentriert" ein Signalanteil ist:
//   - hohe Kohaerenz (Phasenkorrelation zwischen L und R) UND
//   - aehnlicher Pegel in L und R
//   => Anteil wird als "Center" gewertet und aus beiden Kanaelen entfernt.
//
// Center-Signal wird als Mono-Signal interpretiert, das in beiden Kanaelen
// mit voller Amplitude erscheint (Phantom-Center-Annahme), daher gilt:
//   Lonly = L - Center
//   Ronly = R - Center
// und bei gainL = gainC = gainR = 1 ergibt die Summe wieder das Original.
//
// Implementiert als Weighted-Overlap-Add STFT mit sqrt-Hann-Fenstern,
// 4-fachem Overlap (75%). Latenz = fftSize Samples, wird vom Prozessor
// via getLatencySamples() an den Host gemeldet.
class StereoSTFTExtractor
{
public:
    void prepare (double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        fft = std::make_unique<juce::dsp::FFT> (fftOrder);

        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)
        {
            float hann = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (fftSize - 1));
            window[(size_t) i] = std::sqrt (hann);
        }

        // Normierungsfaktor fuer WOLA-Rekonstruktion bei diesem Overlap-Grad.
        // WICHTIG: hier muss der Fensterwert an der tatsaechlich verschobenen
        // Position (idx = i + shift) einfliessen, nicht window[i] selbst -
        // sonst ist das Ergebnis um einen festen Faktor falsch (frueherer
        // Bug: exakt Faktor 2 / 6.02 dB zu leise).
        std::vector<float> ola ((size_t) fftSize, 0.0f);
        for (int shift = -fftSize; shift <= fftSize; shift += hopSize)
            for (int i = 0; i < fftSize; ++i)
            {
                int idx = i + shift;
                if (idx >= 0 && idx < fftSize)
                    ola[(size_t) i] += window[(size_t) idx] * window[(size_t) idx];
            }
        normFactor = juce::jmax (1.0e-6f, ola[(size_t) (fftSize / 2)]);

        fifoInL.assign ((size_t) fftSize, 0.0f);
        fifoInR.assign ((size_t) fftSize, 0.0f);
        fifoPos = 0;

        ringSize = fftSize * 3;
        outL.assign ((size_t) ringSize, 0.0f);
        outC.assign ((size_t) ringSize, 0.0f);
        outR.assign ((size_t) ringSize, 0.0f);
        writeHead = 0;
        // BUG-FIX (User: "Galaxy Latenz nicht korrekt wenn der Focus-Knopf an
        // ist - da hoere ich eine Dopplung"). Der alte Startwert fftSize war
        // falsch herum gerechnet: die Leseposition landete dadurch NICHT
        // fftSize hinter der Schreibposition, sondern 2816 Samples (2,75 x
        // fftSize) - gemessen mit tools/latency-test. Der Prozessor meldete
        // dem Host aber 1024 und verzoegerte den Dry-Pfad ebenfalls um 1024.
        //
        // Ohne den Focus-Filter faellt das nicht auf: bei voll aufgedrehtem
        // Galaxy ist wetGain = 1, damit kuerzt sich der Dry-Anteil in
        // "dry + (wet - dry) * wg" exakt weg. Erst der Focus-Filter mischt
        // beide wieder zusammen - und 1792 Samples Versatz (37 ms bei 48 kHz)
        // sind dann als klarer Doppelschlag zu hoeren.
        //
        // Herleitung: Ringposition p traegt immer den Zeitpunkt p - (fftSize -
        // hopSize). Damit beim Sample n der Zeitpunkt n - fftSize gelesen wird,
        // muss der Startwert ringSize - hopSize sein.
        outReadPos = (ringSize - hopSize) % ringSize;
        samplesUntilNextFrame = hopSize;
    }

    int getLatencySamples() const { return fftSize; }

    // sensitivity: 0..1 (0 = nur sehr strikt zentrierte Anteile, 1 = grosszuegigere Extraktion)
    inline void processSample (float lIn, float rIn, float sensitivity,
                                float& lOnlyOut, float& centerOut, float& rOnlyOut) noexcept
    {
        fifoInL[(size_t) fifoPos] = lIn;
        fifoInR[(size_t) fifoPos] = rIn;
        fifoPos = (fifoPos + 1) % fftSize;

        lOnlyOut  = outL[(size_t) outReadPos];
        centerOut = outC[(size_t) outReadPos];
        rOnlyOut  = outR[(size_t) outReadPos];
        outL[(size_t) outReadPos] = 0.0f;
        outC[(size_t) outReadPos] = 0.0f;
        outR[(size_t) outReadPos] = 0.0f;
        outReadPos = (outReadPos + 1) % ringSize;

        if (--samplesUntilNextFrame == 0)
        {
            samplesUntilNextFrame = hopSize;
            runFrame (sensitivity);
        }
    }

    void reset()
    {
        std::fill (fifoInL.begin(), fifoInL.end(), 0.0f);
        std::fill (fifoInR.begin(), fifoInR.end(), 0.0f);
        std::fill (outL.begin(), outL.end(), 0.0f);
        std::fill (outC.begin(), outC.end(), 0.0f);
        std::fill (outR.begin(), outR.end(), 0.0f);
        fifoPos = 0;
        writeHead = 0;
        outReadPos = (ringSize - hopSize) % ringSize;   // siehe prepare()
        samplesUntilNextFrame = hopSize;
    }

private:
    void runFrame (float sensitivity)
    {
        std::vector<float> frameL ((size_t) fftSize), frameR ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)
        {
            int idx = (fifoPos + i) % fftSize;
            frameL[(size_t) i] = fifoInL[(size_t) idx] * window[(size_t) i];
            frameR[(size_t) i] = fifoInR[(size_t) idx] * window[(size_t) i];
        }

        std::vector<float> bufL ((size_t) fftSize * 2, 0.0f);
        std::vector<float> bufR ((size_t) fftSize * 2, 0.0f);
        for (int i = 0; i < fftSize; ++i)
        {
            bufL[(size_t) i * 2] = frameL[(size_t) i];
            bufR[(size_t) i * 2] = frameR[(size_t) i];
        }

        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (bufL.data()),
                       reinterpret_cast<juce::dsp::Complex<float>*> (bufL.data()), false);
        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (bufR.data()),
                       reinterpret_cast<juce::dsp::Complex<float>*> (bufR.data()), false);

        std::vector<float> cBuf ((size_t) fftSize * 2, 0.0f);
        std::vector<float> lBuf ((size_t) fftSize * 2, 0.0f);
        std::vector<float> rBuf ((size_t) fftSize * 2, 0.0f);

        float power = juce::jmap (juce::jlimit (0.0f, 1.0f, sensitivity), 0.0f, 1.0f, 3.0f, 0.5f);

        for (int b = 0; b < fftSize; ++b)
        {
            float lre = bufL[(size_t) b * 2], lim = bufL[(size_t) b * 2 + 1];
            float rre = bufR[(size_t) b * 2], rim = bufR[(size_t) b * 2 + 1];

            float lmag = std::sqrt (lre * lre + lim * lim);
            float rmag = std::sqrt (rre * rre + rim * rim);

            float corr = (lre * rre + lim * rim) / (lmag * rmag + 1.0e-9f);
            float levelDiff = std::abs (lmag - rmag) / (lmag + rmag + 1.0e-9f);

            float g = juce::jlimit (0.0f, 1.0f, corr) * (1.0f - levelDiff);
            g = std::pow (g, power);

            float cre = g * 0.5f * (lre + rre);
            float cim = g * 0.5f * (lim + rim);

            cBuf[(size_t) b * 2] = cre;
            cBuf[(size_t) b * 2 + 1] = cim;
            lBuf[(size_t) b * 2] = lre - cre;
            lBuf[(size_t) b * 2 + 1] = lim - cim;
            rBuf[(size_t) b * 2] = rre - cre;
            rBuf[(size_t) b * 2 + 1] = rim - cim;
        }

        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (cBuf.data()),
                       reinterpret_cast<juce::dsp::Complex<float>*> (cBuf.data()), true);
        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (lBuf.data()),
                       reinterpret_cast<juce::dsp::Complex<float>*> (lBuf.data()), true);
        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (rBuf.data()),
                       reinterpret_cast<juce::dsp::Complex<float>*> (rBuf.data()), true);

        for (int i = 0; i < fftSize; ++i)
        {
            int idx = (writeHead + i) % ringSize;
            float w = window[(size_t) i] / normFactor;
            outC[(size_t) idx] += cBuf[(size_t) i * 2] * w;
            outL[(size_t) idx] += lBuf[(size_t) i * 2] * w;
            outR[(size_t) idx] += rBuf[(size_t) i * 2] * w;
        }
        writeHead = (writeHead + hopSize) % ringSize;
    }

    static constexpr int fftOrder = 10;
    static constexpr int fftSize  = 1 << fftOrder; // 1024
    static constexpr int hopSize  = fftSize / 4;    // 256 (75% Overlap)

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    float normFactor = 1.0f;
    double sampleRate = 44100.0;

    std::vector<float> fifoInL, fifoInR;
    int fifoPos = 0;

    int ringSize = 0;
    std::vector<float> outL, outC, outR;
    int writeHead = 0;
    int outReadPos = 0;
    int samplesUntilNextFrame = hopSize;
};
