#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "echoshade/DSPPipeline.hpp"
#include <vector>
#include <cstring>

using namespace echoshade;
using Catch::Approx;

class DoublerStage final : public DSPStage {
public:
    void process(float* samples, int frameCount, int channels,
                 double /*sampleRate*/) noexcept override {
        const int n = frameCount * channels;
        for (int i = 0; i < n; ++i)
            samples[i] *= 2.0f;
    }
    void reset() noexcept override {}
    const char* name() const noexcept override { return "Doubler"; }
};

class AdderStage final : public DSPStage {
public:
    explicit AdderStage(float v) : val_(v) {}
    void process(float* samples, int frameCount, int channels,
                 double /*sampleRate*/) noexcept override {
        const int n = frameCount * channels;
        for (int i = 0; i < n; ++i)
            samples[i] += val_;
    }
    void reset() noexcept override {}
    const char* name() const noexcept override { return "Adder"; }
private:
    float val_;
};

TEST_CASE("DSPPipeline empty pipeline is passthrough", "[dsp]") {
    DSPPipeline pipe;
    std::vector<float> buf = {1.0f, 2.0f, 3.0f, 4.0f};
    pipe.process(buf.data(), 4, 1, 48000.0);
    REQUIRE(buf[0] == Approx(1.0f));
    REQUIRE(buf[3] == Approx(4.0f));
}

TEST_CASE("DSPPipeline single doubler stage", "[dsp]") {
    DSPPipeline pipe;
    pipe.addStage(std::make_shared<DoublerStage>());
    std::vector<float> buf = {1.0f, 2.0f, 3.0f};
    pipe.process(buf.data(), 3, 1, 48000.0);
    REQUIRE(buf[0] == Approx(2.0f));
    REQUIRE(buf[1] == Approx(4.0f));
    REQUIRE(buf[2] == Approx(6.0f));
}

TEST_CASE("DSPPipeline two stages apply in order", "[dsp]") {
    // Doubler then Adder(1): result = input*2 + 1
    DSPPipeline pipe;
    pipe.addStage(std::make_shared<DoublerStage>());
    pipe.addStage(std::make_shared<AdderStage>(1.0f));
    std::vector<float> buf = {3.0f};
    pipe.process(buf.data(), 1, 1, 48000.0);
    REQUIRE(buf[0] == Approx(7.0f));   // 3*2+1
}

TEST_CASE("DSPPipeline clearStages removes all stages", "[dsp]") {
    DSPPipeline pipe;
    pipe.addStage(std::make_shared<DoublerStage>());
    pipe.clearStages();
    std::vector<float> buf = {5.0f};
    pipe.process(buf.data(), 1, 1, 48000.0);
    REQUIRE(buf[0] == Approx(5.0f));  // untouched
}

TEST_CASE("DSPPipeline reset calls all stage resets", "[dsp]") {
    DSPPipeline pipe;
    pipe.addStage(std::make_shared<DoublerStage>());
    pipe.resetAll();  // should not throw or crash
    REQUIRE(true);
}
