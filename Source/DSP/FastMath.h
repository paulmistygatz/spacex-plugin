#pragma once
#include <cmath>
#include <array>

// Runde 41 (CPU-Vergleichsbuild SpaceXparaCPU): Sinus/Cosinus ueber eine
// Tabelle mit linearer Interpolation statt std::sin/std::cos pro Sample.
// 4096 Stuetzstellen -> maximaler Fehler ca. 3e-7 (etwa -130 dB), also weit
// unter allem Hoerbaren. Die Phase wird in UMDREHUNGEN angegeben (0..1 =
// 0..2*pi), wie bei den LFOs im Plugin ohnehin gerechnet wird.
#ifndef SPACEX_CPU_OPT
 #define SPACEX_CPU_OPT 1
#endif

namespace spacex
{
    struct SinTable
    {
        static constexpr int kSize = 4096;
        std::array<float, kSize + 1> v {};
        SinTable()
        {
            for (int i = 0; i <= kSize; ++i)
                v[(size_t) i] = (float) std::sin (6.283185307179586 * (double) i / (double) kSize);
        }
    };

    inline const SinTable& sinTable() { static const SinTable t; return t; }

    // sin(2*pi*cycles) fuer beliebige cycles (auch negativ / > 1).
    inline float fastSinCycles (double cycles) noexcept
    {
        double p = cycles - std::floor (cycles);                 // 0..1
        const double x = p * (double) SinTable::kSize;
        int i = (int) x;
        if (i >= SinTable::kSize) i = SinTable::kSize - 1;
        const float f = (float) (x - (double) i);
        const auto& t = sinTable().v;
        return t[(size_t) i] + (t[(size_t) i + 1] - t[(size_t) i]) * f;
    }
    inline float fastCosCycles (double cycles) noexcept { return fastSinCycles (cycles + 0.25); }
}
