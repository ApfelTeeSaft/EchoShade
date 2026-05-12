#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "echoshade/PsychoacousticMasker.hpp"
#include <vector>
#include <cmath>
#include <complex>
#include <numeric>
#include <algorithm>

using namespace echoshade;
using Catch::Approx;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr int    kFFTSize   = 1024;
static constexpr double kSampleRate = 48000.0;
static constexpr int    kNumBins   = kFFTSize / 2 + 1;

TEST_CASE("ATH is positive for all audible frequencies", "[psycho][ath]") {
    for (int hz = 20; hz <= 20000; hz += 100)
        REQUIRE(PsychoacousticMasker::athAmplitude(static_cast<double>(hz)) > 0.0f);
}

TEST_CASE("ATH has minimum near 3-4 kHz (most sensitive region)", "[psycho][ath]") {
    float min_ath = std::numeric_limits<float>::max();
    int   min_hz  = 0;
    for (int hz = 100; hz <= 16000; hz += 50) {
        const float a = PsychoacousticMasker::athAmplitude(hz);
        if (a < min_ath) { min_ath = a; min_hz = hz; }
    }
    INFO("ATH minimum at " << min_hz << " Hz, amplitude = " << min_ath);
    REQUIRE(min_hz >= 2000);
    REQUIRE(min_hz <= 5000);
}

TEST_CASE("ATH is higher (less sensitive) at low and very high frequencies", "[psycho][ath]") {
    const float ath_100   = PsychoacousticMasker::athAmplitude(100.0);
    const float ath_1000  = PsychoacousticMasker::athAmplitude(1000.0);
    const float ath_3500  = PsychoacousticMasker::athAmplitude(3500.0);
    const float ath_15000 = PsychoacousticMasker::athAmplitude(15000.0);

    REQUIRE(ath_100   > ath_1000);
    REQUIRE(ath_1000  > ath_3500);
    REQUIRE(ath_15000 > ath_3500);
}

TEST_CASE("PsychoacousticMasker constructs without throw", "[psycho]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);
    REQUIRE(m.numBins() == kNumBins);
    REQUIRE(m.intensity() == Approx(1.0f));
}

TEST_CASE("Intensity setter clamps to [0, 1]", "[psycho]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);
    m.setIntensity(2.0f);
    REQUIRE(m.intensity() == Approx(1.0f));
    m.setIntensity(-1.0f);
    REQUIRE(m.intensity() == Approx(0.0f));
    m.setIntensity(0.5f);
    REQUIRE(m.intensity() == Approx(0.5f));
}

TEST_CASE("Threshold is positive for any input", "[psycho][threshold]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);
    std::vector<float> mag(kNumBins, 0.0f);
    std::vector<float> thr(kNumBins, 0.0f);
    m.computeThreshold(mag.data(), thr.data());
    for (int k = 0; k < kNumBins; ++k)
        REQUIRE(thr[k] > 0.0f);
}

TEST_CASE("Threshold scales with intensity", "[psycho][threshold]") {
    PsychoacousticMasker m1(kFFTSize, kSampleRate);
    PsychoacousticMasker m2(kFFTSize, kSampleRate);

    std::vector<float> mag(kNumBins, 0.3f);
    std::vector<float> thr1(kNumBins), thr2(kNumBins);

    m1.setIntensity(1.0f);
    m2.setIntensity(0.5f);
    m1.computeThreshold(mag.data(), thr1.data());
    m2.computeThreshold(mag.data(), thr2.data());

    for (int k = 0; k < kNumBins; ++k)
        REQUIRE(thr2[k] == Approx(thr1[k] * 0.5f).margin(1e-7f));
}

TEST_CASE("Threshold is higher at frequencies where signal is louder", "[psycho][threshold]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float> mag(kNumBins, 0.0f);
    for (int k = 10; k <= 30; ++k)
        mag[k] = 0.8f;

    std::vector<float> thr_loud(kNumBins, 0.0f);
    m.computeThreshold(mag.data(), thr_loud.data());

    PsychoacousticMasker m2(kFFTSize, kSampleRate);
    std::vector<float> mag_silent(kNumBins, 0.0f);
    std::vector<float> thr_silent(kNumBins, 0.0f);
    m2.computeThreshold(mag_silent.data(), thr_silent.data());

    float maxLoud   = 0.0f, maxSilent = 0.0f;
    for (int k = 10; k <= 30; ++k) {
        if (thr_loud[k]   > maxLoud)   maxLoud   = thr_loud[k];
        if (thr_silent[k] > maxSilent) maxSilent = thr_silent[k];
    }
    INFO("Threshold in signal band: loud=" << maxLoud << " silent=" << maxSilent);
    REQUIRE(maxLoud > maxSilent);
}

TEST_CASE("Threshold exhibits upward spread (masker raises threshold above)", "[psycho][threshold]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float> mag(kNumBins, 0.0f);
    for (int k = 18; k <= 24; ++k)
        mag[k] = 0.9f;

    std::vector<float> thr(kNumBins, 0.0f);
    for (int i = 0; i < 5; ++i)
        m.computeThreshold(mag.data(), thr.data());

    const float thr_above = thr[40];
    const float thr_below = thr[3];
    INFO("Threshold 1.875kHz=" << thr_above << " 141Hz=" << thr_below);
    REQUIRE(thr_above > thr_below);
}

TEST_CASE("Threshold decays after masker removed (temporal forward masking)", "[psycho][temporal]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float> mag_loud(kNumBins, 0.0f);
    for (int k = 20; k <= 30; ++k) mag_loud[k] = 0.9f;

    std::vector<float> thr(kNumBins, 0.0f);

    for (int i = 0; i < 10; ++i)
        m.computeThreshold(mag_loud.data(), thr.data());

    const float thr_during = thr[25];

    std::vector<float> mag_silent(kNumBins, 0.0f);
    float thr_after = thr_during;
    for (int i = 0; i < 20; ++i) {
        m.computeThreshold(mag_silent.data(), thr.data());
        thr_after = thr[25];
    }

    INFO("Threshold during=" << thr_during << " after 20 frames of silence=" << thr_after);
    REQUIRE(thr_after < thr_during);
}


TEST_CASE("clampToThreshold: bins within threshold are unchanged", "[psycho][clamp]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float>              thr(kNumBins, 0.1f);
    std::vector<std::complex<float>> perturb(kNumBins, {0.05f, 0.0f});

    const std::vector<std::complex<float>> orig = perturb;
    m.clampToThreshold(perturb.data(), thr.data(), kNumBins);

    for (int k = 0; k < kNumBins; ++k)
        REQUIRE(std::abs(perturb[k]) == Approx(std::abs(orig[k])).margin(1e-7f));
}

TEST_CASE("clampToThreshold: bins exceeding threshold are clamped", "[psycho][clamp]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float>              thr(kNumBins, 0.1f);
    std::vector<std::complex<float>> perturb(kNumBins, {0.5f, 0.0f});

    m.clampToThreshold(perturb.data(), thr.data(), kNumBins);

    for (int k = 0; k < kNumBins; ++k)
        REQUIRE(std::abs(perturb[k]) == Approx(0.1f).margin(1e-6f));
}

TEST_CASE("clampToThreshold: phase is preserved during clamping", "[psycho][clamp]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float> thr(kNumBins, 0.1f);
    std::vector<std::complex<float>> perturb(kNumBins);

    for (int k = 0; k < kNumBins; ++k)
        perturb[k] = {0.5f * std::cos(k * 0.3f), 0.5f * std::sin(k * 0.3f)};

    m.clampToThreshold(perturb.data(), thr.data(), kNumBins);

    for (int k = 0; k < kNumBins; ++k) {
        const float expected_phase = std::atan2(std::sin(k * 0.3f), std::cos(k * 0.3f));
        const float actual_phase   = std::atan2(perturb[k].imag(), perturb[k].real());
        REQUIRE(actual_phase == Approx(expected_phase).margin(1e-5f));
    }
}

TEST_CASE("clampToThreshold: returns peak ratio before clamping", "[psycho][clamp]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float>              thr(kNumBins, 0.1f);
    std::vector<std::complex<float>> perturb(kNumBins, {0.0f, 0.0f});
    perturb[10] = {0.5f, 0.0f};

    const float ratio = m.clampToThreshold(perturb.data(), thr.data(), kNumBins);
    REQUIRE(ratio == Approx(5.0f).margin(1e-5f));
}

TEST_CASE("Perturbation energy after clamping is below masking threshold", "[psycho][integration]") {
    PsychoacousticMasker m(kFFTSize, kSampleRate);

    std::vector<float> mag(kNumBins);
    for (int k = 0; k < kNumBins; ++k)
        mag[k] = (k == 0) ? 0.1f : 0.1f / std::sqrt(static_cast<float>(k));

    std::vector<float> thr(kNumBins);
    for (int i = 0; i < 5; ++i)
        m.computeThreshold(mag.data(), thr.data());

    std::vector<std::complex<float>> perturb(kNumBins);
    for (int k = 0; k < kNumBins; ++k)
        perturb[k] = {10.0f * thr[k], 0.0f};

    m.clampToThreshold(perturb.data(), thr.data(), kNumBins);

    for (int k = 0; k < kNumBins; ++k) {
        const float amp = std::abs(perturb[k]);
        REQUIRE(amp <= thr[k] + 1e-6f);
    }
}