#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <memory>
#include <algorithm>
namespace juce {
template<typename T> struct MathConstants { static constexpr T pi = (T) 3.14159265358979323846; };
template<typename T> T jmax (T a, T b) { return a > b ? a : b; }
template<typename T> T jmin (T a, T b) { return a < b ? a : b; }
template<typename T> T jlimit (T lo, T hi, T v) { return v < lo ? lo : (v > hi ? hi : v); }
template<typename T> T jmap (T v, T s0, T s1, T d0, T d1) { return d0 + (d1 - d0) * ((v - s0) / (s1 - s0)); }
namespace dsp {
template<typename T> using Complex = std::complex<T>;
// Naive DFT - langsam, aber exakt dieselbe Mathematik wie juce::dsp::FFT.
class FFT {
public:
    explicit FFT (int order) : n (1 << order) {}
    void perform (const Complex<float>* in, Complex<float>* out, bool inverse) const {
        std::vector<Complex<float>> tmp ((size_t) n);
        const double sign = inverse ? 2.0 * M_PI / n : -2.0 * M_PI / n;
        for (int k = 0; k < n; ++k) {
            std::complex<double> acc (0.0, 0.0);
            for (int t = 0; t < n; ++t) {
                const double a = sign * (double) k * (double) t;
                acc += std::complex<double> ((double) in[t].real(), (double) in[t].imag())
                     * std::complex<double> (std::cos (a), std::sin (a));
            }
            if (inverse) acc /= (double) n;                 // JUCE skaliert die inverse FFT mit 1/N
            tmp[(size_t) k] = Complex<float> ((float) acc.real(), (float) acc.imag());
        }
        std::copy (tmp.begin(), tmp.end(), out);
    }
private:
    int n;
};
}}
