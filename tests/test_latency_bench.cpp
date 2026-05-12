#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "echoshade/DSPPipeline.hpp"
#include "utils/RingBuffer.hpp"
#include "utils/MathUtils.hpp"
#include "utils/VUMeter.hpp"
#include <vector>
#include <numeric>
#include <chrono>
#include <cmath>

using namespace echoshade;
using namespace std::chrono;

static constexpr int   kSampleRate    = 48000;
static constexpr int   kFramesPerBuf  = 512;
static constexpr int   kChannels      = 1;

class GainStage final : public DSPStage {
public:
    explicit GainStage(float g) : gain_(g) {}
    void process(float* s, int frames, int ch, double) noexcept override {
        const int n = frames * ch;
        for (int i = 0; i < n; ++i) s[i] *= gain_;
    }
    void reset() noexcept override {}
    const char* name() const noexcept override { return "Gain"; }
private:
    float gain_;
};

class SineAdderStage final : public DSPStage {
public:
    void process(float* s, int frames, int ch, double sr) noexcept override {
        const int n = frames * ch;
        for (int i = 0; i < n; ++i) {
            phase_ += 2.0f * 3.14159265f * 440.0f / static_cast<float>(sr);
            if (phase_ > 2.0f * 3.14159265f) phase_ -= 2.0f * 3.14159265f;
            s[i] += 0.001f * std::sin(phase_);
        }
    }
    void reset() noexcept override { phase_ = 0.0f; }
    const char* name() const noexcept override { return "SineAdder"; }
private:
    float phase_ = 0.0f;
};

TEST_CASE("DSP empty pipeline processes 512 frames in <1 ms", "[latency]") {
    DSPPipeline pipe;
    std::vector<float> buf(kFramesPerBuf * kChannels, 0.5f);

    constexpr int kRuns = 100;
    double totalMs = 0.0;

    for (int r = 0; r < kRuns; ++r) {
        const auto t0 = steady_clock::now();
        pipe.process(buf.data(), kFramesPerBuf, kChannels,
                     static_cast<double>(kSampleRate));
        const auto t1 = steady_clock::now();
        totalMs += duration<double, std::milli>(t1 - t0).count();
    }

    const double avgMs = totalMs / kRuns;
    INFO("Empty pipeline avg: " << avgMs << " ms");
    REQUIRE(avgMs < 1.0);
}

TEST_CASE("DSP 8-stage pipeline processes 512 frames within real-time budget", "[latency]") {
    DSPPipeline pipe;
    for (int i = 0; i < 4; ++i)
        pipe.addStage(std::make_shared<GainStage>(0.999f));
    for (int i = 0; i < 4; ++i)
        pipe.addStage(std::make_shared<SineAdderStage>());

    std::vector<float> buf(kFramesPerBuf * kChannels, 0.5f);

    constexpr int kRuns = 200;
    double totalMs = 0.0;
    double maxMs   = 0.0;

    for (int r = 0; r < kRuns; ++r) {
        const auto t0 = steady_clock::now();
        pipe.process(buf.data(), kFramesPerBuf, kChannels,
                     static_cast<double>(kSampleRate));
        const auto t1 = steady_clock::now();
        const double ms = duration<double, std::milli>(t1 - t0).count();
        totalMs += ms;
        if (ms > maxMs) maxMs = ms;
    }

    const double avgMs = totalMs / kRuns;
    // Real-time budget for 512 frames @ 48 kHz = 10.67 ms.
    // We target keeping DSP to <3 ms (leaving >70% headroom).
    INFO("8-stage pipeline avg: " << avgMs << " ms, max: " << maxMs << " ms");
    REQUIRE(avgMs < 3.0);
    REQUIRE(maxMs < 10.0);  // no single callback can miss deadline
}

TEST_CASE("VUMeter produces correct dB values", "[latency][meter]") {
    VUMeter meter;

    // Full-scale sine at 0 dBFS peak: with attack coeff 0.99 applied per block,
    // we need enough blocks for the exponential to converge from 0 to ~1.
    // After n blocks: level equals about 1 - 0.99^n  -> n=500 gives ≈ 0.9934 (-0.057 dB).
    constexpr int N = 512;   // one audio buffer
    std::vector<float> sine(N);
    for (int i = 0; i < N; ++i)
        sine[static_cast<std::size_t>(i)] = std::sin(2.0f * 3.14159f * 1000.0f * i / 48000.0f);

    for (int b = 0; b < 500; ++b)
        meter.update(sine.data(), N);

    INFO("Peak dB after 500 blocks: " << meter.peakDb());
    REQUIRE(meter.peakDb() > -3.0f);  // should be close to 0 dBFS now
    REQUIRE(meter.peakDb() <= 0.5f);  // not exceeding

    // Silence decays peak exponentially (release = 0.995/block).
    // 0.995^1000 equals about 0.0067; run 1200 blocks to be safely below 0.01.
    std::vector<float> silence(N, 0.0f);
    for (int b = 0; b < 1200; ++b)
        meter.update(silence.data(), N);
    INFO("Peak after 1200 silence blocks: " << meter.peakDb());
    REQUIRE(meter.peakLinear() < 0.01f);
}

TEST_CASE("RingBuffer audio throughput > 10x real-time", "[latency]") {
    AudioRingBuffer rb(8192);
    std::vector<float> block(512, 1.0f);

    constexpr int kIter = 10000;
    const auto t0 = steady_clock::now();
    for (int i = 0; i < kIter; ++i) {
        rb.write(block.data(), block.size());
        rb.read(block.data(), block.size());
    }
    const auto t1 = steady_clock::now();

    const double totalMs = duration<double, std::milli>(t1 - t0).count();
    const double msPerOp = totalMs / kIter;
    // A single write+read of 512 floats must take < 0.1 ms to be RT-safe.
    INFO("RingBuffer write+read 512 floats: " << msPerOp << " ms avg");
    REQUIRE(msPerOp < 0.1);
}

TEST_CASE("MathUtils Bark/Mel conversions are fast", "[latency]") {
    constexpr int kN = 100'000;
    const auto t0 = steady_clock::now();
    float acc = 0.0f;
    for (int i = 0; i < kN; ++i)
        acc += math::hzToBark(static_cast<float>(100 + i % 8000));
    const auto t1 = steady_clock::now();
    (void)acc;

    const double ms = duration<double, std::milli>(t1 - t0).count();
    INFO("hzToBark x100k: " << ms << " ms");
    REQUIRE(ms < 50.0);
}