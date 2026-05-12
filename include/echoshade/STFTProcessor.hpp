#pragma once
#include <vector>
#include <complex>
#include <functional>
#include <cstdint>

namespace echoshade {

enum class WindowType { Hann, BlackmanHarris, Kaiser };

struct STFTConfig {
    int        fftSize    = 1024;
    int        hopSize    = 256;   // 75% overlap by default (4× oversampling)
    WindowType windowType = WindowType::Hann;
    double     kaiserBeta = 8.0;   // shape parameter, only for Kaiser window
};

class STFTProcessor {
public:
    using SpectralCallback = std::function<void(std::complex<float>* spectrum,
                                                int numBins,
                                                double sampleRate)>;

    explicit STFTProcessor(const STFTConfig& cfg);
    ~STFTProcessor();

    STFTProcessor(const STFTProcessor&)            = delete;
    STFTProcessor& operator=(const STFTProcessor&) = delete;

    int fftSize() const noexcept;
    int hopSize() const noexcept;
    int numBins() const noexcept;

    double binFrequency(int bin, double sampleRate) const noexcept;

    void pushSamples(const float*            samples,
                     int                      count,
                     const SpectralCallback&  spectralCb,
                     double                   sampleRate) noexcept;

    int pullSamples(float* out, int maxCount) noexcept;

    int available() const noexcept;

    void reset() noexcept;

    static std::vector<float> makeWindow(WindowType type, int size,
                                         double kaiserBeta = 8.0);

    static std::vector<float> computeNormEnv(const std::vector<float>& window,
                                              int hopSize);

private:
    struct Impl;
    Impl* impl_ = nullptr;
};

} // namespace echoshade