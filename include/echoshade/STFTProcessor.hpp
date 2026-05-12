#pragma once
#include <vector>
#include <complex>
#include <cstdint>

namespace echoshade {

enum class WindowType { Hann, BlackmanHarris, Kaiser };

struct STFTConfig {
    int        fftSize     = 1024;
    int        hopSize     = 256;
    WindowType windowType  = WindowType::Hann;
    double     kaiserBeta  = 8.0;   // only for Kaiser window
};

/// Short-Time Fourier Transform analysis + synthesis via overlap-add.
/// Holds one frame's worth of complex spectrum for DSP stages to modify.
class STFTProcessor {
public:
    explicit STFTProcessor(const STFTConfig& cfg);
    ~STFTProcessor();

    STFTProcessor(const STFTProcessor&)            = delete;
    STFTProcessor& operator=(const STFTProcessor&) = delete;

    int fftSize() const;
    int hopSize() const;
    int numBins() const;   // fftSize/2 + 1

    /// Push one hop of time-domain samples (frameCount == hopSize).
    /// When a full frame is available, calls the provided spectral callback
    /// with the complex spectrum, then synthesises output via OLA.
    using SpectralCallback = void(*)(std::complex<float>* spectrum,
                                     int numBins, void* userData);

    void pushSamples(const float* samples, int count,
                     SpectralCallback cb, void* userData);

    /// Pull synthesised output samples (frameCount == hopSize per call to push).
    int  pullSamples(float* out, int maxCount);

    void reset();

private:
    struct Impl;
    Impl* impl_ = nullptr;
};

} // namespace echoshade
