#include "juceshim.h"
#define JUCE_DSP_H_INCLUDED
#include "LCRExtractor_local.h"
#include <cstdio>
#include <cmath>
int main() {
    StereoSTFTExtractor ex;
    ex.prepare (48000.0);
    ex.setExtractionRange (20.0f, 22000.0f);          // Bass-Guard aus: reine Identitaet pruefen
    const int lat = ex.getLatencySamples();
    const int N   = lat * 6;
    const int imp = lat;
    std::vector<float> in ((size_t) N, 0.0f);
    in[(size_t) imp] = 1.0f;                          // Impuls, identisch auf L und R

    int bestIdx = -1; float bestVal = 0.0f;
    double energy = 0.0, worstIdentity = 0.0;
    std::vector<float> leftOut ((size_t) N, 0.0f);
    for (int i = 0; i < N; ++i) {
        float lo, c, ro;
        ex.processSample (in[(size_t) i], in[(size_t) i], 0.5f, lo, c, ro);
        // Bei gain 1/1/1 muss L-only + Center exakt das verzoegerte L sein.
        const float l = lo + c;
        leftOut[(size_t) i] = l;
        energy += (double) l * l;
        const float want = (i - lat >= 0 && i - lat < N) ? in[(size_t) (i - lat)] : 0.0f;
        worstIdentity = std::max (worstIdentity, (double) std::fabs (l - want));
        if (std::fabs (l) > std::fabs (bestVal)) { bestVal = l; bestIdx = i; }
    }
    printf ("fftSize / Latenz     : %d Samples (%.1f ms @48k)\n", lat, 1000.0 * lat / 48000.0);
    printf ("Impuls rein bei      : %d\n", imp);
    printf ("Maximum raus bei     : %d  (Wert %.5f)\n", bestIdx, bestVal);
    printf ("=> gemessene Latenz  : %d Samples\n", bestIdx - imp);
    printf ("   gemeldete Latenz  : %d Samples   %s\n", lat,
            (bestIdx - imp) == lat ? "OK" : "<<< FEHLER");
    printf ("Gesamtenergie raus   : %.5f  (rein: 1.0)\n", energy);
    printf ("Groesster Fehler L   : %.3e  %s\n", worstIdentity,
            worstIdentity < 1.0e-5 ? "(bitgenaue Rekonstruktion)" : "<<< FEHLER");
    printf ("\nUmgebung des Maximums:\n");
    for (int i = bestIdx - 3; i <= bestIdx + 3; ++i)
        if (i >= 0 && i < N) printf ("  [%6d] %+.6f%s\n", i, leftOut[(size_t) i], i == bestIdx ? "   <= max" : "");
    return 0;
}
