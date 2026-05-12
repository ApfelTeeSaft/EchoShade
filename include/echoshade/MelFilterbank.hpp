#pragma once
#include <vector>
#include <cstdint>

namespace echoshade {

class MelFilterbank {
public:
    MelFilterbank(int numFilters, int fftSize, double sampleRate,
                  double fMinHz = 0.0, double fMaxHz = 0.0);

    void apply(const float* magnitudes, int numBins,
               float* melEnergy) const noexcept;

    int numFilters() const noexcept { return static_cast<int>(filters_.size()); }
    int fftSize()    const noexcept { return fftSize_; }
    int numBins()    const noexcept { return fftSize_ / 2 + 1; }

    const std::vector<double>& centerFreqsHz() const noexcept { return centerFreqsHz_; }

private:
    struct Filter {
        int   binStart;
        int   binPeak;
        int   binEnd;
        std::vector<float> weights;
    };

    int                 fftSize_;
    std::vector<Filter> filters_;
    std::vector<double> centerFreqsHz_;
};

} // namespace echoshade