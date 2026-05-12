#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "utils/MathUtils.hpp"
#include <cmath>

using namespace echoshade::math;
using Catch::Approx;

TEST_CASE("ampToDb / dbToAmp round-trip", "[math]") {
    for (float db : {-60.f, -20.f, -6.f, 0.f, 6.f, 20.f}) {
        float amp = dbToAmp(db);
        float back = ampToDb(amp);
        REQUIRE(back == Approx(db).margin(0.001f));
    }
}

TEST_CASE("ampToDb floor at -200 dB for zero input", "[math]") {
    float db = ampToDb(0.0f);
    REQUIRE(db < -100.0f);  // clamped to 1e-10 -> -200 dB
}

TEST_CASE("hzToBark monotonic", "[math]") {
    float prev = hzToBark(100.f);
    for (float hz = 200.f; hz <= 10000.f; hz += 200.f) {
        float bark = hzToBark(hz);
        REQUIRE(bark > prev);
        prev = bark;
    }
}

TEST_CASE("hzToMel / melToHz round-trip", "[math]") {
    for (float hz : {100.f, 440.f, 1000.f, 4000.f, 8000.f}) {
        float mel  = hzToMel(hz);
        float back = melToHz(mel);
        REQUIRE(back == Approx(hz).margin(0.01f));
    }
}

TEST_CASE("clamp", "[math]") {
    REQUIRE(clamp(5.0f, 0.0f, 1.0f) == 1.0f);
    REQUIRE(clamp(-1.0f, 0.0f, 1.0f) == 0.0f);
    REQUIRE(clamp(0.5f, 0.0f, 1.0f) == 0.5f);
}

TEST_CASE("nextPow2", "[math]") {
    REQUIRE(nextPow2(0)  == 1);
    REQUIRE(nextPow2(1)  == 1);
    REQUIRE(nextPow2(3)  == 4);
    REQUIRE(nextPow2(16) == 16);
    REQUIRE(nextPow2(17) == 32);
    REQUIRE(nextPow2(1023) == 1024);
}

TEST_CASE("lerp", "[math]") {
    REQUIRE(lerp(0.f, 1.f, 0.5f) == Approx(0.5f));
    REQUIRE(lerp(2.f, 4.f, 0.25f) == Approx(2.5f));
}

TEST_CASE("rms", "[math]") {
    float buf[] = {1.0f, -1.0f, 1.0f, -1.0f};
    REQUIRE(rms(buf, 4) == Approx(1.0f));

    float zeros[8] = {};
    REQUIRE(rms(zeros, 8) == Approx(0.0f));
}
