#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "echoshade/STFTProcessor.hpp"
#include "echoshade/STFTStage.hpp"
#include "echoshade/MelFilterbank.hpp"
#include <vector>
#include <cmath>
#include <numeric>
#include <complex>

using namespace echoshade;
using Catch::Approx;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TEST_CASE("Hann window: zero at n=0, peak at N/2", "[stft][window]") {
    auto w = STFTProcessor::makeWindow(WindowType::Hann, 1024);
    REQUIRE(w.size() == 1024);
    REQUIRE(w[0]   == Approx(0.0f).margin(1e-6f));
    REQUIRE(w[512] == Approx(1.0f).margin(1e-5f));
    const float peak = *std::max_element(w.begin(), w.end());
    REQUIRE(peak == Approx(1.0f).margin(1e-5f));
}

TEST_CASE("Blackman-Harris window: near-zero at n=0, peak near 1", "[stft][window]") {
    auto w = STFTProcessor::makeWindow(WindowType::BlackmanHarris, 512);
    REQUIRE(w[0] < 1e-3f);
    const float peak = *std::max_element(w.begin(), w.end());
    REQUIRE(peak == Approx(1.0f).margin(1e-4f));
}

TEST_CASE("Kaiser window beta=0 approximates rectangular", "[stft][window]") {
    auto w = STFTProcessor::makeWindow(WindowType::Kaiser, 256, 0.0);
    for (float v : w)
        REQUIRE(v == Approx(1.0f).margin(1e-5f));
}

TEST_CASE("Kaiser window beta=8 endpoints are small", "[stft][window]") {
    auto w = STFTProcessor::makeWindow(WindowType::Kaiser, 512, 8.0);
    REQUIRE(w[0]   < 0.01f);
    REQUIRE(w[511] < 0.01f);
    const float peak = *std::max_element(w.begin(), w.end());
    REQUIRE(peak == Approx(1.0f).margin(1e-4f));
}

TEST_CASE("OLA normEnv Hann 50% overlap sums to correct value", "[stft][ola]") {
    const int fftSize = 1024;
    const int hopSize = 512;
    auto w = STFTProcessor::makeWindow(WindowType::Hann, fftSize);
    auto env = STFTProcessor::computeNormEnv(w, hopSize);
    REQUIRE(static_cast<int>(env.size()) == hopSize);
    const float first = env[0];
    REQUIRE(first > 0.0f);
    for (float v : env)
        REQUIRE(v == Approx(first).margin(first * 0.01f));
}

TEST_CASE("OLA normEnv Hann 75% overlap is positive", "[stft][ola]") {
    const int fftSize = 1024;
    const int hopSize = 256;
    auto w = STFTProcessor::makeWindow(WindowType::Hann, fftSize);
    auto env = STFTProcessor::computeNormEnv(w, hopSize);
    REQUIRE(static_cast<int>(env.size()) == hopSize);
    for (float v : env)
        REQUIRE(v > 0.0f);
}

TEST_CASE("STFTProcessor::binFrequency returns correct Hz", "[stft][bins]") {
    STFTConfig cfg;
    cfg.fftSize = 1024;
    cfg.hopSize = 256;
    STFTProcessor proc(cfg);

    REQUIRE(proc.numBins() == 513);
    REQUIRE(proc.binFrequency(0,  48000.0) == Approx(0.0));
    REQUIRE(proc.binFrequency(1,  48000.0) == Approx(48000.0 / 1024.0));
    REQUIRE(proc.binFrequency(512, 48000.0) == Approx(24000.0));
}

static std::vector<float> makeSine(int N, double freqHz, double sampleRate, float amplitude = 1.0f) {
    std::vector<float> s(N);
    for (int i = 0; i < N; ++i)
        s[i] = amplitude * static_cast<float>(std::sin(2.0 * M_PI * freqHz * i / sampleRate));
    return s;
}

TEST_CASE("STFT passthrough round-trip energy preservation", "[stft][roundtrip]") {
    STFTConfig cfg;
    cfg.fftSize    = 1024;
    cfg.hopSize    = 256;
    cfg.windowType = WindowType::Hann;

    STFTProcessor proc(cfg);

    const int kSR = 48000;
    const int kSamples = kSR;
    auto input = makeSine(kSamples, 440.0, kSR, 0.5f);

    proc.pushSamples(input.data(), kSamples, nullptr, kSR);

    const int avail = proc.available();
    REQUIRE(avail > 0);

    std::vector<float> output(avail);
    const int got = proc.pullSamples(output.data(), avail);
    REQUIRE(got == avail);

    const int skip = cfg.fftSize;
    if (got <= skip) return;

    const int start = skip;
    const int len   = std::min(got - skip, kSamples - skip);

    double inEnergy  = 0.0, outEnergy = 0.0;
    for (int i = 0; i < len; ++i) {
        inEnergy  += static_cast<double>(input[start + i])  * input[start + i];
        outEnergy += static_cast<double>(output[i])         * output[i];
    }

    INFO("inEnergy=" << inEnergy << " outEnergy=" << outEnergy);
    REQUIRE(outEnergy > inEnergy * 0.80);
    REQUIRE(outEnergy < inEnergy * 1.20);
}

TEST_CASE("STFT reset clears state", "[stft]") {
    STFTConfig cfg;
    cfg.fftSize = 512;
    cfg.hopSize = 128;
    STFTProcessor proc(cfg);

    auto signal = makeSine(4096, 1000.0, 48000.0, 0.5f);
    proc.pushSamples(signal.data(), 4096, nullptr, 48000.0);
    REQUIRE(proc.available() > 0);

    proc.reset();
    REQUIRE(proc.available() == 0);
}

TEST_CASE("STFT spectral callback receives correct bin count", "[stft][callback]") {
    STFTConfig cfg;
    cfg.fftSize = 512;
    cfg.hopSize = 128;
    STFTProcessor proc(cfg);

    int callbackBins = -1;
    auto cb = [&](std::complex<float>* /*spec*/, int numBins, double /*sr*/) {
        callbackBins = numBins;
    };

    auto signal = makeSine(2048, 440.0, 48000.0, 0.3f);
    proc.pushSamples(signal.data(), static_cast<int>(signal.size()), cb, 48000.0);

    REQUIRE(callbackBins == proc.numBins());
    REQUIRE(callbackBins == 257);
}

TEST_CASE("STFT spectral zeroing produces silence", "[stft][callback]") {
    STFTConfig cfg;
    cfg.fftSize = 512;
    cfg.hopSize = 128;
    STFTProcessor proc(cfg);

    auto zeroCb = [](std::complex<float>* spec, int numBins, double) {
        for (int i = 0; i < numBins; ++i) spec[i] = {0.0f, 0.0f};
    };

    auto signal = makeSine(4096, 440.0, 48000.0, 0.8f);
    proc.pushSamples(signal.data(), static_cast<int>(signal.size()), zeroCb, 48000.0);

    std::vector<float> out(proc.available());
    proc.pullSamples(out.data(), static_cast<int>(out.size()));

    float maxAbs = 0.0f;
    for (float v : out) maxAbs = std::max(maxAbs, std::abs(v));
    INFO("Max abs after zeroing: " << maxAbs);
    REQUIRE(maxAbs < 1e-4f);
}

TEST_CASE("MelFilterbank constructs without throw", "[mel]") {
    MelFilterbank fb(40, 1024, 48000.0);
    REQUIRE(fb.numFilters() == 40);
    REQUIRE(fb.fftSize()    == 1024);
    REQUIRE(fb.numBins()    == 513);
}

TEST_CASE("MelFilterbank apply produces positive energy for non-zero spectrum", "[mel]") {
    const int fftSize = 1024;
    MelFilterbank fb(40, fftSize, 48000.0);

    std::vector<float> mag(fftSize / 2 + 1, 1.0f);
    std::vector<float> mels(fb.numFilters(), 0.0f);
    fb.apply(mag.data(), static_cast<int>(mag.size()), mels.data());

    for (float v : mels)
        REQUIRE(v >= 0.0f);

    const float total = std::accumulate(mels.begin(), mels.end(), 0.0f);
    REQUIRE(total > 0.0f);
}

TEST_CASE("MelFilterbank center frequencies are monotonically increasing", "[mel]") {
    MelFilterbank fb(40, 1024, 48000.0);
    const auto& freqs = fb.centerFreqsHz();
    for (int i = 1; i < static_cast<int>(freqs.size()); ++i)
        REQUIRE(freqs[i] > freqs[i - 1]);
}

TEST_CASE("MelFilterbank apply returns zero for silent spectrum", "[mel]") {
    MelFilterbank fb(32, 512, 16000.0);
    std::vector<float> mag(fb.numBins(), 0.0f);
    std::vector<float> mels(fb.numFilters(), -1.0f);
    fb.apply(mag.data(), static_cast<int>(mag.size()), mels.data());
    for (float v : mels)
        REQUIRE(v == Approx(0.0f).margin(1e-9f));
}

TEST_CASE("STFTStage passthrough does not crash", "[stftstage]") {
    STFTConfig cfg;
    cfg.fftSize = 512;
    cfg.hopSize = 128;
    STFTStage stage(cfg);

    std::vector<float> buf(512, 0.5f);
    for (int i = 0; i < 20; ++i)
        stage.process(buf.data(), 512, 1, 48000.0);
    REQUIRE(true);
}