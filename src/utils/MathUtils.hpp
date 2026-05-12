#pragma once
#include <cmath>
#include <cstdint>
#include <numbers>
#include <algorithm>

namespace echoshade::math {

constexpr float kPi    = std::numbers::pi_v<float>;
constexpr float k2Pi   = 2.0f * kPi;
constexpr float kSqrt2 = std::numbers::sqrt2_v<float>;

/// Convert amplitude ratio to dB.
inline float ampToDb(float amp) noexcept {
    return 20.0f * std::log10(std::max(amp, 1e-10f));
}

/// Convert dB to amplitude ratio.
inline float dbToAmp(float db) noexcept {
    return std::pow(10.0f, db / 20.0f);
}

/// Convert frequency (Hz) to Bark scale.
inline float hzToBark(float hz) noexcept {
    return 13.0f * std::atan(0.00076f * hz)
         + 3.5f  * std::atan((hz / 7500.0f) * (hz / 7500.0f));
}

/// Convert Bark to approximate Hz.
inline float barkToHz(float bark) noexcept {
    return 1960.0f * (bark + 0.53f) / (26.28f - bark);
}

/// Convert frequency (Hz) to mel scale.
inline float hzToMel(float hz) noexcept {
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

/// Convert mel to Hz.
inline float melToHz(float mel) noexcept {
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

/// Fast approximation of atan2 (error < 0.01 rad).
inline float fastAtan2(float y, float x) noexcept {
    const float absY = std::fabs(y) + 1e-20f;
    float r, angle;
    if (x < 0.0f) {
        r     = (x + absY) / (absY - x);
        angle = 0.75f * kPi;
    } else {
        r     = (x - absY) / (x + absY);
        angle = 0.25f * kPi;
    }
    angle += (0.1963f * r * r - 0.9817f) * r;
    return (y < 0.0f) ? -angle : angle;
}

/// Linear interpolation.
inline float lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

/// Clamp value into [lo, hi].
template <typename T>
inline T clamp(T v, T lo, T hi) noexcept {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

/// Next power of two >= n.
inline std::uint32_t nextPow2(std::uint32_t n) noexcept {
    if (n == 0) return 1;
    --n;
    n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
    return n + 1;
}

/// Compute RMS of a float buffer.
inline float rms(const float* buf, int n) noexcept {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += buf[i] * buf[i];
    return std::sqrt(sum / static_cast<float>(n));
}

} // namespace echoshade::math
