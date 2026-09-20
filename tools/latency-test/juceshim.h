#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <memory>
#include <atomic>
#include <algorithm>
namespace juce {
template<typename T> struct MathConstants { static constexpr T pi = (T) 3.14159265358979323846; };
template<typename T> T jmax (T a, T b) { return a > b ? a : b; }
template<typename T> T jmin (T a, T b) { return a < b ? a : b; }
template<typename T> T jlimit (T lo, T hi, T v) { return v < lo ? lo : (v > hi ? hi : v); }
template<typename T> T jmap (T v, T s0, T s1, T d0, T d1) { return d0 + (d1 - d0) * ((v - s0) / (s1 - s0)); }
namespace dsp {
template<typename T> using Complex = std::complex<T>;
// Iterative Radix-2-FFT (Cooley-Tukey), doppelte Genauigkeit.
// Gleiche Konvention wie juce::dsp::FFT: vorwaerts exp(-2*pi*i*k*t/N),
// rueckwaerts exp(+2*pi*i*k*t/N) MIT 1/N-Skalierung.
// (Vorher stand hier eine naive DFT - bei fftSize 4096 waere die 16-mal
// langsamer als bei 1024 pro Transformation und der Test praktisch
// unbenutzbar.)
class FFT {
public:
    explicit FFT (int order) : n (1 << order) {}
    void perform (const Complex<float>* in, Complex<float>* out, bool inverse) const {
        std::vector<std::complex<double>> a ((size_t) n);
        for (int i = 0; i < n; ++i) a[(size_t) i] = { (double) in[i].real(), (double) in[i].imag() };

        for (int i = 1, j = 0; i < n; ++i) {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[(size_t) i], a[(size_t) j]);
        }
        const double sign = inverse ? 1.0 : -1.0;
        for (int len = 2; len <= n; len <<= 1) {
            const double ang = sign * 2.0 * M_PI / (double) len;
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (int i = 0; i < n; i += len) {
                std::complex<double> w (1.0, 0.0);
                for (int k = 0; k < len / 2; ++k) {
                    const std::complex<double> u = a[(size_t) (i + k)];
                    const std::complex<double> v = a[(size_t) (i + k + len / 2)] * w;
                    a[(size_t) (i + k)]             = u + v;
                    a[(size_t) (i + k + len / 2)]   = u - v;
                    w *= wl;
                }
            }
        }
        if (inverse) for (auto& z : a) z /= (double) n;
        for (int i = 0; i < n; ++i) out[i] = Complex<float> ((float) a[(size_t) i].real(),
                                                             (float) a[(size_t) i].imag());
    }
private:
    int n;
};
}}
