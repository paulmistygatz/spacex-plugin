#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>

namespace mistycat
{
    // Einfacher Ringpuffer Audio -> GUI fuer das Goniometer. Der Audio-Thread
    // schreibt nur, wenn Platz ist (sonst werden Samples verworfen - fuer eine
    // Anzeige egal), blockiert nie und reserviert keinen Speicher.
    class MeterFifo
    {
    public:
        static constexpr int kSize = 16384;

        MeterFifo() : fifo (kSize), bufL ((size_t) kSize, 0.0f), bufR ((size_t) kSize, 0.0f) {}

        void push (const float* l, const float* r, int n) noexcept
        {
            int s1, n1, s2, n2;
            fifo.prepareToWrite (n, s1, n1, s2, n2);
            for (int i = 0; i < n1; ++i) { bufL[(size_t) (s1 + i)] = l[i];      bufR[(size_t) (s1 + i)] = r[i]; }
            for (int i = 0; i < n2; ++i) { bufL[(size_t) (s2 + i)] = l[n1 + i]; bufR[(size_t) (s2 + i)] = r[n1 + i]; }
            fifo.finishedWrite (n1 + n2);
        }

        // Liest bis zu maxN Samples, gibt die Anzahl zurueck.
        int pull (float* l, float* r, int maxN) noexcept
        {
            int s1, n1, s2, n2;
            fifo.prepareToRead (maxN, s1, n1, s2, n2);
            for (int i = 0; i < n1; ++i) { l[i] = bufL[(size_t) (s1 + i)];      r[i] = bufR[(size_t) (s1 + i)]; }
            for (int i = 0; i < n2; ++i) { l[n1 + i] = bufL[(size_t) (s2 + i)]; r[n1 + i] = bufR[(size_t) (s2 + i)]; }
            fifo.finishedRead (n1 + n2);
            return n1 + n2;
        }

        void clear() noexcept { fifo.reset(); }

    private:
        juce::AbstractFifo fifo;
        std::vector<float> bufL, bufR;
    };
}
