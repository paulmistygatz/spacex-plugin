#include "juceshim.h"
#define JUCE_DSP_H_INCLUDED
namespace juce_dsp_stub {}
#include "LCRExtractor_local.h"
#include <cstdio>
int main() {
    StereoSTFTExtractor ex;
    ex.prepare (48000.0);
    const int N = 1024 * 8;
    std::vector<float> in ((size_t) N, 0.0f);
    in[(size_t) 1024] = 1.0f;                 // Impuls bei Sample 1024
    int bestIdx = -1; float bestVal = 0.0f; double energy = 0.0;
    std::vector<float> sum ((size_t) N, 0.0f);
    for (int i = 0; i < N; ++i) {
        float lo, c, ro;
        ex.processSample (in[(size_t) i], in[(size_t) i], 0.5f, lo, c, ro);
        const float s = lo + c + ro;          // bei gain 1/1/1 muss das wieder das Original sein
        sum[(size_t) i] = s;
        energy += (double) s * s;
        if (std::fabs (s) > std::fabs (bestVal)) { bestVal = s; bestIdx = i; }
    }
    printf ("Impuls rein bei      : 1024\n");
    printf ("Maximum raus bei     : %d  (Wert %.4f)\n", bestIdx, bestVal);
    printf ("=> gemessene Latenz  : %d Samples\n", bestIdx - 1024);
    printf ("   gemeldete Latenz  : %d Samples\n", ex.getLatencySamples());
    printf ("Gesamtenergie raus   : %.4f  (rein: 1.0)\n", energy);
    printf ("\nUmgebung des Maximums:\n");
    for (int i = bestIdx - 3; i <= bestIdx + 3; ++i)
        if (i >= 0 && i < N) printf ("  [%5d] %+.5f%s\n", i, sum[(size_t) i], i == bestIdx ? "   <= max" : "");
    return 0;
}
