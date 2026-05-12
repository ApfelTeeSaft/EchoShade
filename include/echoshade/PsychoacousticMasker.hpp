#pragma once
#include <vector>
#include <complex>

namespace echoshade {

/// Computes the psychoacoustic masking threshold (simplified ISO 532-B / Zwicker model).
/// Given an input power spectrum, returns the per-bin masking threshold in linear amplitude.
/// Perturbations below this threshold are imperceptible.
class PsychoacousticMasker {
public:
    explicit PsychoacousticMasker(int fftSize, double sampleRate);
    ~PsychoacousticMasker();

    PsychoacousticMasker(const PsychoacousticMasker&)            = delete;
    PsychoacousticMasker& operator=(const PsychoacousticMasker&) = delete;

    /// Compute masking threshold from input magnitude spectrum.
    /// @param magnitudes  per-bin magnitude (linear), length = numBins
    /// @param threshold   output per-bin threshold (linear amplitude), length = numBins
    void computeThreshold(const float* magnitudes, float* threshold) noexcept;

    int numBins() const;

private:
    struct Impl;
    Impl* impl_ = nullptr;
};

} // namespace echoshade
