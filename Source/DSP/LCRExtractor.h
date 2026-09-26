#pragma once
#include "FastMath.h"   // nur fuer SPACEX_CPU_OPT
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include <cmath>
#include <memory>

// ============================================================================
// Stereo -> LCR Extraktion (Phantom-Center-Prinzip, STFT/WOLA)
// ============================================================================
//
// Pro Frequenzbin wird bestimmt, wie "zentriert" ein Signalanteil ist:
//   - Phasengleichheit zwischen L und R
//   - Kohaerenz (Stabilitaet dieser Phasenbeziehung ueber die Zeit)
//   - aehnlicher Pegel in L und R
// Der so gewichtete Anteil wandert in den Center-Kanal und wird aus L/R
// entfernt:   Lonly = L - C,  Ronly = R - C   =>  L + 0 + R == Original.
//
// ---------------------------------------------------------------------------
// UMBAU (Runde 32) - drei Gruende, warum das vorher schlechter klang als
// Bertom Phantom Center (User: "viel weicher, weniger Bassresonanzen"):
//
// 1) AUFLOESUNG. fftSize war 1024 = 46,9 Hz pro Bin bei 48 kHz. Im Bass
//    liegen damit mehrere Partialtoene in EINEM Bin, die Center/Seiten-
//    Entscheidung wird fuer alle gemeinsam getroffen und schmiert. Jetzt
//    4096 = 11,7 Hz pro Bin. Das entspricht auch der Beobachtung, dass
//    Bertom rund die vierfache Latenz von Galaxy hatte.
//
// 2) KEINE GLAETTUNG. Korrelation und Pegeldifferenz wurden aus EINEM
//    einzelnen FFT-Frame geschaetzt - das ist eine extrem verrauschte
//    Schaetzung. Der Bin-Gain sprang damit von Frame zu Frame, also mit
//    der Hop-Rate. Das ist eine Amplitudenmodulation mit ~47 Hz (bei
//    4096/1024) auf JEDEM Partialton, und genau das hoert man als
//    "Resonanzen" / metallisch. Jetzt werden die Kreuz- und
//    Autospektren ueber die Zeit geglaettet (echte Kohaerenzschaetzung,
//    Zeitkonstante ueber setSmoothing()), zusaetzlich wird der fertige
//    Gain ueber die Frequenz geglaettet (vorwaerts/rueckwaerts, also
//    phasenneutral).
//
// 3) ALLOKATIONEN IM AUDIO-THREAD. runFrame() legte pro Frame acht
//    std::vector an. Alle Arbeitspuffer sind jetzt Member und werden in
//    prepare() einmal dimensioniert.
//
// Dazu zwei Extras:
//
// * NUR NOCH DREI FFTs statt sechs. Frueher wurden L-only, Center und
//   R-only einzeln ruecktransformiert. Weil aber Lonly = L - C gilt und
//   das trockene L/R ohnehin verzoegert vorliegt, reicht EINE inverse
//   FFT (Center); L-only und R-only entstehen durch Subtraktion vom
//   verzoegerten Original. Halbe CPU - und die Seitenanteile erben keine
//   Fenster-/Overlap-Fehler mehr, die Summe ist konstruktionsbedingt
//   exakt das Original.
//
// * EXTRAKTIONSBAND (setExtractionRange). Wie bei Leapwing CenterOne /
//   Bertom: nur innerhalb des Bandes wird in Center und Seiten zerlegt,
//   ausserhalb bleibt das Material unangetastet in L/R. Als Maske pro Bin
//   INNERHALB der FFT - damit linearphasig und summentreu, im Gegensatz
//   zum frueheren Biquad-Bass-Guard, der auf der Differenz sass und die
//   Phase verbog.
//
// Latenz = fftSize Samples, gemeldet ueber getLatencySamples().
// ============================================================================
class StereoSTFTExtractor
{
public:
    void prepare (double sampleRateIn)
    {
        sampleRate = (sampleRateIn > 0.0 ? sampleRateIn : 48000.0);
        fft = std::make_unique<juce::dsp::FFT> (fftOrder);

        // PERIODISCHES Hann (Nenner fftSize, nicht fftSize-1): nur damit
        // erfuellt sqrt-Hann bei 75% Overlap die COLA-Bedingung exakt.
        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)
        {
            const float hann = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi
                                                        * (float) i / (float) fftSize);
            window[(size_t) i] = std::sqrt (hann);
        }

        // Normierung fuer die WOLA-Rekonstruktion. Der Fensterwert muss an
        // der tatsaechlich verschobenen Position stehen (idx = i + shift) -
        // sonst ist das Ergebnis um einen festen Faktor falsch (frueherer
        // Bug: exakt Faktor 2 / 6,02 dB zu leise).
        {
            std::vector<float> ola ((size_t) fftSize, 0.0f);
            for (int shift = -fftSize; shift <= fftSize; shift += hopSize)
                for (int i = 0; i < fftSize; ++i)
                {
                    const int idx = i + shift;
                    if (idx >= 0 && idx < fftSize)
                        ola[(size_t) i] += window[(size_t) idx] * window[(size_t) idx];
                }
            normFactor = juce::jmax (1.0e-6f, ola[(size_t) (fftSize / 2)]);
        }

        fifoInL.assign ((size_t) fftSize, 0.0f);
        fifoInR.assign ((size_t) fftSize, 0.0f);
        fifoPos = 0;

        // Trockenes L/R, exakt um die Analyselatenz verzoegert. Daraus
        // entstehen die Seitenanteile per Subtraktion (siehe oben).
        dryL.assign ((size_t) fftSize, 0.0f);
        dryR.assign ((size_t) fftSize, 0.0f);
        dryPos = 0;

        ringSize = fftSize * 3;
        outC.assign ((size_t) ringSize, 0.0f);
        writeHead = 0;
        // Ringposition p traegt immer den Zeitpunkt p - (fftSize - hopSize).
        // Damit beim Sample n der Zeitpunkt n - fftSize gelesen wird, muss
        // der Startwert ringSize - hopSize sein. (Alter Wert fftSize ergab
        // 2,75 x fftSize echte Latenz bei gemeldeten fftSize - hoerbar als
        // Dopplung, sobald sich verarbeiteter und trockener Pfad mischten.)
        outReadPos = (ringSize - hopSize) % ringSize;
        samplesUntilNextFrame = hopSize;

        bufL.assign ((size_t) fftSize * 2, 0.0f);
        bufR.assign ((size_t) fftSize * 2, 0.0f);
        cBuf.assign ((size_t) fftSize * 2, 0.0f);
        fftOut.assign ((size_t) fftSize * 2, 0.0f);

        const size_t nb = (size_t) (fftSize / 2 + 1);
        sxx.assign  (nb, 0.0f);
        syy.assign  (nb, 0.0f);
        sxyRe.assign(nb, 0.0f);
        sxyIm.assign(nb, 0.0f);
        gain.assign (nb, 0.0f);
        mask.assign (nb, 1.0f);

        maskDirty.store (true);
        updateSmoothingCoeff();
    }

    int getLatencySamples() const { return fftSize; }

    // --- Parameter (Message-Thread) -----------------------------------------

    // 0 = kaum Glaettung (schnell, aber rauh), 1 = sehr traege.
    // Entspricht Bertoms "Smooth". Default 0.5.
    void setSmoothing (float s01)
    {
        const float v = juce::jlimit (0.0f, 1.0f, s01);
        if (std::abs (v - smoothAmount) > 1.0e-4f)
        {
            smoothAmount = v;
            updateSmoothingCoeff();
        }
    }

    // Extraktionsband. Nur hier drin wird in Center/Seiten zerlegt.
    // loHz <= 20 bzw. hiHz >= sr/2 heisst "keine Begrenzung".
    void setExtractionRange (float loHzIn, float hiHzIn)
    {
        const float lo = juce::jlimit (10.0f, 2000.0f, loHzIn);
        const float hi = juce::jmax (lo * 2.0f, hiHzIn);
        if (std::abs (lo - loHz) > 0.01f || std::abs (hi - hiHz) > 0.01f)
        {
            loHz = lo; hiHz = hi;
            maskDirty.store (true);
        }
    }

    // --- Audio-Thread --------------------------------------------------------

    // sensitivity: 0..1 (0 = nur sehr strikt zentrierte Anteile,
    //                    1 = grosszuegigere Extraktion)
    inline void processSample (float lIn, float rIn, float sensitivity,
                               float& lOnlyOut, float& centerOut, float& rOnlyOut) noexcept
    {
        fifoInL[(size_t) fifoPos] = lIn;
        fifoInR[(size_t) fifoPos] = rIn;
       #if SPACEX_CPU_OPT
        if (++fifoPos == fftSize) fifoPos = 0;
       #else
        fifoPos = (fifoPos + 1) % fftSize;
       #endif

        // Verzoegertes Original (genau fftSize Samples: erst lesen, dann
        // an dieselbe Stelle schreiben).
        const float dl = dryL[(size_t) dryPos];
        const float dr = dryR[(size_t) dryPos];
        dryL[(size_t) dryPos] = lIn;
        dryR[(size_t) dryPos] = rIn;
       #if SPACEX_CPU_OPT
        if (++dryPos == fftSize) dryPos = 0;
       #else
        dryPos = (dryPos + 1) % fftSize;
       #endif

        const float c = outC[(size_t) outReadPos];
        outC[(size_t) outReadPos] = 0.0f;
       #if SPACEX_CPU_OPT
        if (++outReadPos == ringSize) outReadPos = 0;
       #else
        outReadPos = (outReadPos + 1) % ringSize;
       #endif

        centerOut = c;
        lOnlyOut  = dl - c;
        rOnlyOut  = dr - c;

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
        std::fill (dryL.begin(),    dryL.end(),    0.0f);
        std::fill (dryR.begin(),    dryR.end(),    0.0f);
        std::fill (outC.begin(),    outC.end(),    0.0f);
        std::fill (sxx.begin(),     sxx.end(),     0.0f);
        std::fill (syy.begin(),     syy.end(),     0.0f);
        std::fill (sxyRe.begin(),   sxyRe.end(),   0.0f);
        std::fill (sxyIm.begin(),   sxyIm.end(),   0.0f);
        std::fill (gain.begin(),    gain.end(),    0.0f);
        fifoPos = 0;
        dryPos = 0;
        writeHead = 0;
        outReadPos = (ringSize - hopSize) % ringSize;   // siehe prepare()
        samplesUntilNextFrame = hopSize;
    }

private:
    void updateSmoothingCoeff()
    {
        // Zeitkonstante exponentiell: 0 -> 3 ms, 0.5 -> ~19 ms, 1 -> 120 ms.
        // Ueber die Zeit gerechnet, damit sich bei 96 kHz nichts aendert.
        const double tauSec = 0.003 * std::pow (40.0, (double) smoothAmount);
        const double hopSec = (double) hopSize / sampleRate;
        specAlpha = (float) std::exp (-hopSec / juce::jmax (1.0e-4, tauSec));
    }

    void rebuildMask()
    {
        const float binHz = (float) (sampleRate / (double) fftSize);
        const int   nb    = fftSize / 2;
        const float edge  = 1.41421356f;                      // halbe Oktave Flanke
        const float invLogEdge = 1.0f / std::log (edge);
        const bool  cutLo = (loHz > 20.5f);
        const bool  cutHi = (hiHz < (float) (0.49 * sampleRate));

        for (int b = 0; b <= nb; ++b)
        {
            const float f = (float) b * binHz;
            float m = 1.0f;

            if (cutLo)
            {
                const float a = loHz / edge;
                if (f <= a)            m = 0.0f;
                else if (f < loHz)     m = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi
                                                                   * std::log (f / a) * invLogEdge);
            }
            // Obere Bandgrenze (AIR): Butterworth 2. Ordnung als Betrag,
            // also -3 dB bei hiHz und 12 dB pro Oktave darueber - dieselbe
            // Kurve, die Bertom Phantom Center zeigt (User-Messung mit EQ
            // Curve Analyzer). Vorher fiel die Maske innerhalb einer halben
            // Oktave auf null; das klang haerter. Bleibt linearphasig und
            // summentreu, weil es nur eine Gewichtung pro Bin ist.
            if (cutHi && m > 0.0f)
            {
                const float r = f / hiHz;
                m *= 1.0f / std::sqrt (1.0f + r * r * r * r);
            }
            mask[(size_t) b] = m;
        }
    }

    void runFrame (float sensitivity)
    {
        if (maskDirty.exchange (false))
            rebuildMask();

        // --- Analyse ---------------------------------------------------------
       #if SPACEX_CPU_OPT
        // CPU-Build (Runde 41): L und R in EINER komplexen FFT (L = Real-,
        // R = Imaginaerteil) statt zwei getrennten. Mathematisch dieselben
        // Spektren, nur Rundungsunterschiede im Bereich 1e-7:
        //   L[k] = (Z[k] + conj Z[N-k]) / 2,  R[k] = (Z[k] - conj Z[N-k]) / 2j
        {
            int idx = fifoPos;
            for (int i = 0; i < fftSize; ++i)
            {
                cBuf[(size_t) i * 2]     = fifoInL[(size_t) idx] * window[(size_t) i];
                cBuf[(size_t) i * 2 + 1] = fifoInR[(size_t) idx] * window[(size_t) i];
                if (++idx == fftSize) idx = 0;
            }
            // Review 1.0.1: out-of-place (siehe fftOut unten).
            fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (cBuf.data()),
                          reinterpret_cast<juce::dsp::Complex<float>*> (fftOut.data()), false);
            const int nbHalf = fftSize / 2;
            for (int b = 0; b <= nbHalf; ++b)
            {
                const int m = (b == 0) ? 0 : fftSize - b;
                const float zr = fftOut[(size_t) b * 2], zi = fftOut[(size_t) b * 2 + 1];
                const float wr = fftOut[(size_t) m * 2], wi = fftOut[(size_t) m * 2 + 1];
                bufL[(size_t) b * 2]     = 0.5f * (zr + wr);
                bufL[(size_t) b * 2 + 1] = 0.5f * (zi - wi);
                bufR[(size_t) b * 2]     = 0.5f * (zi + wi);
                bufR[(size_t) b * 2 + 1] = -0.5f * (zr - wr);
            }
        }
       #else
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        for (int i = 0; i < fftSize; ++i)
        {
            const int idx = (fifoPos + i) % fftSize;
            bufL[(size_t) i * 2] = fifoInL[(size_t) idx] * window[(size_t) i];
            bufR[(size_t) i * 2] = fifoInR[(size_t) idx] * window[(size_t) i];
        }

        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (bufL.data()),
                      reinterpret_cast<juce::dsp::Complex<float>*> (bufL.data()), false);
        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (bufR.data()),
                      reinterpret_cast<juce::dsp::Complex<float>*> (bufR.data()), false);
       #endif

        const int   nb    = fftSize / 2;
        const float a     = specAlpha;
        const float oneMa = 1.0f - a;
        // Gravity-Kennlinie. Der NEUTRALE Punkt (power = 1) ist die
        // mathematisch exakte Zerlegung: fuer gleichphasiges Material ist
        // C dann genau min(|L|,|R|), L-only und R-only tragen den Rest.
        // Der gehoert auf die Mittelstellung - der Regler rastet dort ein
        // und hat centerOut-Optik. Die alte lineare Abbildung 3.0..0.5
        // setzte power = 1 erst bei 80% und liess darunter nur zu
        // vorsichtige Extraktion zu (User: "Gravity ist gar nicht so
        // relevant wie ich dachte"). Jetzt geometrisch um die Mitte:
        //   0% -> 4.0 (sehr streng)   50% -> 1.0 (exakt)   100% -> 0.25
        const float power = std::pow (4.0f, 1.0f - 2.0f * juce::jlimit (0.0f, 1.0f, sensitivity));
        constexpr float eps = 1.0e-12f;

        // --- Gain pro Bin aus GEGLAETTETEN Spektren --------------------------
        // Das ist der Kern des Umbaus: nicht der Gain wird geglaettet,
        // sondern die Statistik, aus der er entsteht. Damit ist die
        // Kohaerenz eine echte Schaetzung ueber mehrere Frames und kein
        // Einzelframe-Zufallswert mehr.
        for (int b = 0; b <= nb; ++b)
        {
            const float lre = bufL[(size_t) b * 2], lim = bufL[(size_t) b * 2 + 1];
            const float rre = bufR[(size_t) b * 2], rim = bufR[(size_t) b * 2 + 1];

            const size_t s = (size_t) b;
            sxx[s]   = a * sxx[s]   + oneMa * (lre * lre + lim * lim);
            syy[s]   = a * syy[s]   + oneMa * (rre * rre + rim * rim);
            sxyRe[s] = a * sxyRe[s] + oneMa * (lre * rre + lim * rim);
            sxyIm[s] = a * sxyIm[s] + oneMa * (lim * rre - lre * rim);

            const float crossMag = std::sqrt (sxyRe[s] * sxyRe[s] + sxyIm[s] * sxyIm[s]);
            const float lmag     = std::sqrt (sxx[s]);
            const float rmag     = std::sqrt (syy[s]);

            // Kohaerenz: wie stabil ist die Phasenbeziehung ueber die Zeit.
            const float coh   = crossMag / (lmag * rmag + eps);
            // Ausrichtung: cos der mittleren Phasendifferenz (negativ =
            // gegenphasig, gehoert dann nicht in die Mitte).
            const float align = sxyRe[s] / (crossMag + eps);
            // Pegeldifferenz. Fuer gleichphasiges Material ist
            // C = (1 - levelDiff) * 0.5 * (L + R) exakt min(|L|,|R|) -
            // also genau die richtige Zerlegung.
            const float levelDiff = std::abs (lmag - rmag) / (lmag + rmag + eps);

            float g = juce::jlimit (0.0f, 1.0f, align) * juce::jlimit (0.0f, 1.0f, coh)
                        * (1.0f - levelDiff);
           #if SPACEX_CPU_OPT
            if (g > 0.0f && power != 1.0f)   // pow(g, 1) = g: exakt gleich, nur ohne Rechnung
                g = std::pow (g, power);
           #else
            if (g > 0.0f)
                g = std::pow (g, power);
           #endif
            gain[s] = g * mask[s];
        }

        // --- Glaettung ueber die Frequenz (vorwaerts + rueckwaerts) ----------
        // Zwei Durchlaeufe in beide Richtungen: das Ergebnis ist symmetrisch,
        // verschiebt also nichts. Nimmt dem Gain die Bin-zu-Bin-Zacken, die
        // sonst als Verschmierung im Zeitbereich landen.
        {
            constexpr float fb = 0.45f;
            float acc = gain[0];
            for (int b = 1; b <= nb; ++b) { acc = fb * acc + (1.0f - fb) * gain[(size_t) b]; gain[(size_t) b] = acc; }
            acc = gain[(size_t) nb];
            for (int b = nb - 1; b >= 0; --b) { acc = fb * acc + (1.0f - fb) * gain[(size_t) b]; gain[(size_t) b] = acc; }
        }

        // --- Center-Spektrum (konjugiert-symmetrisch gespiegelt) -------------
        for (int b = 0; b <= nb; ++b)
        {
            const float g = gain[(size_t) b] * 0.5f;
            const float cre = g * (bufL[(size_t) b * 2]     + bufR[(size_t) b * 2]);
            const float cim = g * (bufL[(size_t) b * 2 + 1] + bufR[(size_t) b * 2 + 1]);
            cBuf[(size_t) b * 2]     = cre;
            cBuf[(size_t) b * 2 + 1] = cim;

            if (b > 0 && b < nb)
            {
                const int m = fftSize - b;
                cBuf[(size_t) m * 2]     =  cre;
                cBuf[(size_t) m * 2 + 1] = -cim;
            }
        }

        fft->perform (reinterpret_cast<juce::dsp::Complex<float>*> (cBuf.data()),
                      reinterpret_cast<juce::dsp::Complex<float>*> (fftOut.data()), true);

        for (int i = 0; i < fftSize; ++i)
        {
            const int idx = (writeHead + i) % ringSize;
            outC[(size_t) idx] += fftOut[(size_t) i * 2] * (window[(size_t) i] / normFactor);
        }
        writeHead = (writeHead + hopSize) % ringSize;
    }

    static constexpr int fftOrder = 12;
    static constexpr int fftSize  = 1 << fftOrder;  // 4096 -> 11,7 Hz/Bin @48k
    static constexpr int hopSize  = fftSize / 4;    // 1024 (75% Overlap)

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    float normFactor = 1.0f;
    double sampleRate = 48000.0;

    std::vector<float> fifoInL, fifoInR;
    int fifoPos = 0;

    std::vector<float> dryL, dryR;
    int dryPos = 0;

    int ringSize = 0;
    std::vector<float> outC;
    int writeHead = 0;
    int outReadPos = 0;
    int samplesUntilNextFrame = hopSize;

    // Arbeitspuffer - in prepare() dimensioniert, nie im Audio-Thread.
    std::vector<float> bufL, bufR, cBuf;
    // Review 1.0.1: eigener Ausgabepuffer fuer die FFT. juce::dsp::FFT::perform()
    // ist laut JUCE nur OUT-OF-PLACE erlaubt. In-place rechnet nur Apples vDSP
    // zufaellig richtig; der JUCE-Fallback (Windows) lieferte Muell, Galaxy war
    // dort wirkungslos (Center = 0, im Test gemessen).
    std::vector<float> fftOut;
    std::vector<float> sxx, syy, sxyRe, sxyIm, gain, mask;

    float smoothAmount = 0.5f;
    float specAlpha    = 0.6f;

    float loHz = 20.0f, hiHz = 22000.0f;
    std::atomic<bool> maskDirty { true };
};
