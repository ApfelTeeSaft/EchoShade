#pragma once
#include <vector>
#include <complex>
#include <cstdint>

namespace echoshade {

class PsychoacousticMasker {
public:
    explicit PsychoacousticMasker(int fftSize, double sampleRate);
    ~PsychoacousticMasker();

    PsychoacousticMasker(const PsychoacousticMasker&)            = delete;
    PsychoacousticMasker& operator=(const PsychoacousticMasker&) = delete;

    void computeThreshold(const float* magnitudes, float* threshold) noexcept;

    float clampToThreshold(std::complex<float>* perturbation,
                           const float*          threshold,
                           int                   numBins) noexcept;

    static float athAmplitude(double freqHz) noexcept;

    void setIntensity(float intensity) noexcept;
    float intensity() const noexcept;

    int numBins() const noexcept;

private:
    struct Impl;
    Impl* impl_ = nullptr;
};

} // namespace echoshade