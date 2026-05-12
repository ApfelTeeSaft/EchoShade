#include "echoshade/PsychoacousticMasker.hpp"
#include <cmath>
#include <cassert>
#include <algorithm>
#include <vector>
#include <complex>
#include <limits>

namespace echoshade {

static constexpr int    kNumBarkBands   = 24;
static constexpr float  kMinPower       = 1e-12f;
static constexpr float  kDefaultIntens  = 1.0f;

float PsychoacousticMasker::athAmplitude(double freqHz) noexcept {
    if (freqHz < 10.0) freqHz = 10.0;
    const double f = freqHz / 1000.0; // kHz
    const double athDb =  3.64  * std::pow(f, -0.8)
                        - 6.5   * std::exp(-0.6 * (f - 3.3) * (f - 3.3))
                        + 1e-3  * std::pow(f, 4.0);
    const double athDbFS = std::min(athDb - 90.0, 0.0);
    return static_cast<float>(std::pow(10.0, athDbFS / 20.0));
}

struct PsychoacousticMasker::Impl {
    int    fftSize;
    int    numBins;
    double sampleRate;
    float  intensity = kDefaultIntens;

    std::vector<float> athAmpl;
    std::vector<float> binBark;
    std::vector<int>   binBand;

    struct BandInfo {
        int   binFirst;
        int   binLast;
        float barkCenter;
    };
    std::vector<BandInfo> bands;

    float spreadMtx[kNumBarkBands][kNumBarkBands];

    float excitationSmooth[kNumBarkBands] = {};

    float attackCoef  = 0.95f;
    float releaseCoef = 0.85f;

    void init(int fs, double sr) {
        fftSize    = fs;
        numBins    = fs / 2 + 1;
        sampleRate = sr;

        athAmpl.resize(numBins);
        binBark.resize(numBins);
        binBand.resize(numBins, 0);

        for (int k = 0; k < numBins; ++k) {
            const double hz = static_cast<double>(k) * sr / fs;
            binBark[k] = static_cast<float>(hzToBark(hz));
            athAmpl[k] = PsychoacousticMasker::athAmplitude(hz);
        }

        bands.resize(kNumBarkBands);
        for (int b = 0; b < kNumBarkBands; ++b) {
            const float bLo = static_cast<float>(b);
            const float bHi = static_cast<float>(b + 1);
            bands[b].barkCenter = bLo + 0.5f;
            bands[b].binFirst   = numBins;
            bands[b].binLast    = 0;

            for (int k = 0; k < numBins; ++k) {
                if (binBark[k] >= bLo && binBark[k] < bHi) {
                    if (k < bands[b].binFirst) bands[b].binFirst = k;
                    if (k > bands[b].binLast)  bands[b].binLast  = k;
                    binBand[k] = b;
                }
            }
            if (bands[b].binFirst > bands[b].binLast) {
                const int mid = std::min(
                    static_cast<int>((barkToHz((bLo + bHi) * 0.5f) / sr) * fs),
                    numBins - 1);
                bands[b].binFirst = bands[b].binLast = std::max(0, mid);
            }
        }
        for (int k = 0; k < numBins; ++k)
            if (binBark[k] >= static_cast<float>(kNumBarkBands))
                binBand[k] = kNumBarkBands - 1;

        for (int src = 0; src < kNumBarkBands; ++src) {
            for (int dst = 0; dst < kNumBarkBands; ++dst) {
                const float dz = static_cast<float>(dst - src);
                float spreadDb;
                if (dz >= 0.0f)
                    spreadDb = -10.0f * dz;
                else
                    spreadDb = -25.0f * (-dz);
                spreadMtx[src][dst] = std::pow(10.0f, spreadDb / 10.0f);
            }
        }
    }

    static double hzToBark(double hz) noexcept {
        return 13.0 * std::atan(0.00076 * hz)
             + 3.5  * std::atan((hz / 7500.0) * (hz / 7500.0));
    }
    static double barkToHz(double bark) noexcept {
        return 1960.0 * (bark + 0.53) / (26.28 - bark);
    }
};

PsychoacousticMasker::PsychoacousticMasker(int fftSize, double sampleRate) {
    assert(fftSize > 0 && (fftSize & 1) == 0);
    assert(sampleRate > 0.0);
    impl_ = new Impl();
    impl_->init(fftSize, sampleRate);
}

PsychoacousticMasker::~PsychoacousticMasker() {
    delete impl_;
}

int   PsychoacousticMasker::numBins()   const noexcept { return impl_->numBins; }
float PsychoacousticMasker::intensity() const noexcept { return impl_->intensity; }

void PsychoacousticMasker::setIntensity(float i) noexcept {
    impl_->intensity = std::max(0.0f, std::min(1.0f, i));
}

void PsychoacousticMasker::computeThreshold(const float* magnitudes,
                                             float*       threshold) noexcept {
    const int N = impl_->numBins;

    float excitation[kNumBarkBands] = {};
    for (int k = 0; k < N; ++k) {
        const float p = magnitudes[k] * magnitudes[k];
        const int   b = impl_->binBand[k];
        if (p > excitation[b]) excitation[b] = p;
    }

    for (int b = 0; b < kNumBarkBands; ++b) {
        float& sm = impl_->excitationSmooth[b];
        const float coef = (excitation[b] >= sm) ? impl_->attackCoef
                                                  : impl_->releaseCoef;
        sm = coef * sm + (1.0f - coef) * excitation[b];
        excitation[b] = std::max(excitation[b], sm);
    }

    float spread[kNumBarkBands] = {};
    for (int src = 0; src < kNumBarkBands; ++src) {
        const float e = excitation[src];
        if (e < kMinPower) continue;
        for (int dst = 0; dst < kNumBarkBands; ++dst)
            spread[dst] += e * impl_->spreadMtx[src][dst];
    }

    for (int k = 0; k < N; ++k) {
        const int b = impl_->binBand[k];
        const int bandWidth = std::max(1,
            impl_->bands[b].binLast - impl_->bands[b].binFirst + 1);

        const float maskAmp = std::sqrt(spread[b] / static_cast<float>(bandWidth));

        const float ath = impl_->athAmpl[k];
        threshold[k] = std::max(maskAmp, ath) * impl_->intensity;
    }
}

float PsychoacousticMasker::clampToThreshold(std::complex<float>* perturbation,
                                              const float*          threshold,
                                              int                   numBins) noexcept {
    float peakRatio = 0.0f;
    for (int k = 0; k < numBins; ++k) {
        const float amp = std::abs(perturbation[k]);
        const float thr = threshold[k];
        if (thr < 1e-15f) {
            perturbation[k] = {0.0f, 0.0f};
            continue;
        }
        const float ratio = amp / thr;
        if (ratio > peakRatio) peakRatio = ratio;
        if (amp > thr && amp > 0.0f)
            perturbation[k] *= (thr / amp);
    }
    return peakRatio;
}

} // namespace echoshade