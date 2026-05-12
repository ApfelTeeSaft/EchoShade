#include "echoshade/MelFilterbank.hpp"
#include <cmath>
#include <cassert>
#include <algorithm>

namespace echoshade {

// Linear frequency <-> mel scale (O'Shaughnessy formula, 1000 mel = 1000 Hz).
static double hzToMelLocal(double hz)  { return 2595.0 * std::log10(1.0 + hz / 700.0); }
static double melToHzLocal(double mel) { return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0); }

MelFilterbank::MelFilterbank(int numFilters, int fftSize, double sampleRate,
                              double fMinHz, double fMaxHz)
    : fftSize_(fftSize)
{
    assert(numFilters > 0);
    assert(fftSize > 0 && (fftSize & 1) == 0);
    assert(sampleRate > 0.0);

    if (fMaxHz <= 0.0 || fMaxHz > sampleRate / 2.0)
        fMaxHz = sampleRate / 2.0;
    if (fMinHz < 0.0) fMinHz = 0.0;

    const int numBins = fftSize / 2 + 1;
    const double melMin = hzToMelLocal(fMinHz);
    const double melMax = hzToMelLocal(fMaxHz);

    std::vector<double> melPoints(numFilters + 2);
    for (int m = 0; m < numFilters + 2; ++m)
        melPoints[m] = melMin + static_cast<double>(m) / (numFilters + 1) * (melMax - melMin);

    auto melToBin = [&](double mel) -> int {
        const double hz = melToHzLocal(mel);
        return static_cast<int>(std::round(hz / (sampleRate / fftSize)));
    };

    filters_.resize(numFilters);
    centerFreqsHz_.resize(numFilters);

    for (int m = 0; m < numFilters; ++m) {
        const int bLeft   = std::min(melToBin(melPoints[m]),     numBins - 1);
        const int bCenter = std::min(melToBin(melPoints[m + 1]), numBins - 1);
        const int bRight  = std::min(melToBin(melPoints[m + 2]), numBins - 1);

        centerFreqsHz_[m] = melToHzLocal(melPoints[m + 1]);

        Filter& f   = filters_[m];
        f.binStart  = bLeft;
        f.binPeak   = bCenter;
        f.binEnd    = bRight + 1;

        const int len = f.binEnd - f.binStart;
        f.weights.resize(std::max(len, 0), 0.0f);

        for (int b = bLeft; b <= bRight; ++b) {
            float w = 0.0f;
            if (b <= bCenter && bCenter > bLeft)
                w = static_cast<float>(b - bLeft) / (bCenter - bLeft);
            else if (b > bCenter && bRight > bCenter)
                w = static_cast<float>(bRight - b) / (bRight - bCenter);
            const int idx = b - f.binStart;
            if (idx >= 0 && idx < static_cast<int>(f.weights.size()))
                f.weights[idx] = w;
        }
    }
}

void MelFilterbank::apply(const float* magnitudes, int numBins,
                           float* melEnergy) const noexcept {
    const int nf = static_cast<int>(filters_.size());
    for (int m = 0; m < nf; ++m) {
        const Filter& f = filters_[m];
        float sum = 0.0f;
        const int end = std::min(f.binEnd, numBins);
        for (int b = f.binStart; b < end; ++b) {
            const int idx = b - f.binStart;
            if (idx < static_cast<int>(f.weights.size()))
                sum += magnitudes[b] * f.weights[idx];
        }
        melEnergy[m] = sum;
    }
}

} // namespace echoshade