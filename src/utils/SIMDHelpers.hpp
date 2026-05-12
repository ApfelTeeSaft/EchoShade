#pragma once
#include <cstddef>

#if defined(ECHOSHADE_HAS_AVX2)
#  include <immintrin.h>
#  define ECHOSHADE_SIMD_WIDTH 8
#elif defined(ECHOSHADE_HAS_SSE42)
#  include <nmmintrin.h>
#  define ECHOSHADE_SIMD_WIDTH 4
#else
#  define ECHOSHADE_SIMD_WIDTH 1
#endif

#include <cmath>

namespace echoshade::simd {

/// Multiply-add: dst[i] = a[i] * b[i] + c[i], all aligned 16-byte buffers.
inline void multiplyAdd(float* dst, const float* a, const float* b,
                        const float* c, std::size_t n) noexcept {
#if defined(ECHOSHADE_HAS_AVX2)
    const std::size_t blocks = n / 8;
    for (std::size_t i = 0; i < blocks * 8; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_loadu_ps(c + i);
        _mm256_storeu_ps(dst + i, _mm256_fmadd_ps(va, vb, vc));
    }
    for (std::size_t i = blocks * 8; i < n; ++i)
        dst[i] = a[i] * b[i] + c[i];
#elif defined(ECHOSHADE_HAS_SSE42)
    const std::size_t blocks = n / 4;
    for (std::size_t i = 0; i < blocks * 4; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_loadu_ps(c + i);
        _mm_storeu_ps(dst + i, _mm_add_ps(_mm_mul_ps(va, vb), vc));
    }
    for (std::size_t i = blocks * 4; i < n; ++i)
        dst[i] = a[i] * b[i] + c[i];
#else
    for (std::size_t i = 0; i < n; ++i)
        dst[i] = a[i] * b[i] + c[i];
#endif
}

/// Element-wise clamp: dst[i] = clamp(src[i], lo, hi).
inline void clampBuffer(float* dst, const float* src, float lo, float hi,
                        std::size_t n) noexcept {
#if defined(ECHOSHADE_HAS_AVX2)
    const __m256 vlo = _mm256_set1_ps(lo);
    const __m256 vhi = _mm256_set1_ps(hi);
    const std::size_t blocks = n / 8;
    for (std::size_t i = 0; i < blocks * 8; i += 8) {
        __m256 v = _mm256_loadu_ps(src + i);
        v = _mm256_max_ps(v, vlo);
        v = _mm256_min_ps(v, vhi);
        _mm256_storeu_ps(dst + i, v);
    }
    for (std::size_t i = blocks * 8; i < n; ++i)
        dst[i] = src[i] < lo ? lo : (src[i] > hi ? hi : src[i]);
#elif defined(ECHOSHADE_HAS_SSE42)
    const __m128 vlo = _mm_set1_ps(lo);
    const __m128 vhi = _mm_set1_ps(hi);
    const std::size_t blocks = n / 4;
    for (std::size_t i = 0; i < blocks * 4; i += 4) {
        __m128 v = _mm_loadu_ps(src + i);
        v = _mm_max_ps(v, vlo);
        v = _mm_min_ps(v, vhi);
        _mm_storeu_ps(dst + i, v);
    }
    for (std::size_t i = blocks * 4; i < n; ++i)
        dst[i] = src[i] < lo ? lo : (src[i] > hi ? hi : src[i]);
#else
    for (std::size_t i = 0; i < n; ++i)
        dst[i] = src[i] < lo ? lo : (src[i] > hi ? hi : src[i]);
#endif
}

} // namespace echoshade::simd
