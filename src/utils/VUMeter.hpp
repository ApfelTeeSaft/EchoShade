#pragma once
#include <atomic>
#include <cmath>
#include <algorithm>

namespace echoshade {

/// Audio thread calls update(); GUI thread calls snapshot().
/// Uses exponential-decay ballistics so peak hold is smooth.
struct VUMeter {
    explicit VUMeter(float attackCoeff  = 0.99f,
                     float releaseCoeff = 0.995f)
        : attack_(attackCoeff)
        , release_(releaseCoeff)
        , peakLin_(0.0f)
        , rmsLin_(0.0f)
    {}

    void update(const float* samples, int count) noexcept {
        float maxAbs = 0.0f;
        float sumSq  = 0.0f;
        for (int i = 0; i < count; ++i) {
            const float v = samples[i] < 0.0f ? -samples[i] : samples[i];
            if (v > maxAbs) maxAbs = v;
            sumSq += samples[i] * samples[i];
        }
        const float rms  = std::sqrt(sumSq / static_cast<float>(count));
        const float coef = (maxAbs > peakLin_.load(std::memory_order_relaxed))
                           ? attack_ : release_;
        const float newPeak = coef * peakLin_.load(std::memory_order_relaxed)
                            + (1.0f - coef) * maxAbs;
        const float newRms  = coef * rmsLin_.load(std::memory_order_relaxed)
                            + (1.0f - coef) * rms;
        peakLin_.store(newPeak, std::memory_order_relaxed);
        rmsLin_ .store(newRms,  std::memory_order_relaxed);
    }

    float peakLinear() const noexcept {
        return peakLin_.load(std::memory_order_relaxed);
    }
    float rmsLinear() const noexcept {
        return rmsLin_.load(std::memory_order_relaxed);
    }
    float peakDb() const noexcept {
        const float p = peakLin_.load(std::memory_order_relaxed);
        return p > 1e-10f ? 20.0f * std::log10(p) : -96.0f;
    }
    float rmsDb() const noexcept {
        const float r = rmsLin_.load(std::memory_order_relaxed);
        return r > 1e-10f ? 20.0f * std::log10(r) : -96.0f;
    }
    void reset() noexcept {
        peakLin_.store(0.0f, std::memory_order_relaxed);
        rmsLin_ .store(0.0f, std::memory_order_relaxed);
    }

private:
    float                attack_;
    float                release_;
    std::atomic<float>   peakLin_;
    std::atomic<float>   rmsLin_;
};

} // namespace echoshade