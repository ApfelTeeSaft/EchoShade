#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "utils/AtomicParams.hpp"
#include <thread>
#include <atomic>
#include <vector>

using namespace echoshade;
using Catch::Approx;

struct TestParams {
    float intensity = 0.5f;
    int   mode      = 0;
    bool  enabled   = true;
};

TEST_CASE("AtomicParams initial value readable", "[atomicparams]") {
    AtomicParams<TestParams> p({0.75f, 2, false});
    const auto& v = p.read();
    REQUIRE(v.intensity == Approx(0.75f));
    REQUIRE(v.mode      == 2);
    REQUIRE(v.enabled   == false);
}

TEST_CASE("AtomicParams publish makes new value visible", "[atomicparams]") {
    AtomicParams<TestParams> p;
    p.publish({1.0f, 5, false});
    const auto& v = p.read();
    REQUIRE(v.intensity == Approx(1.0f));
    REQUIRE(v.mode      == 5);
    REQUIRE(v.enabled   == false);
}

TEST_CASE("AtomicParams repeated publish and read", "[atomicparams]") {
    AtomicParams<TestParams> p;
    for (int i = 0; i < 1000; ++i) {
        p.publish({static_cast<float>(i) * 0.001f, i, (i % 2 == 0)});
        const auto& v = p.read();
        REQUIRE(v.mode == i);
    }
}

TEST_CASE("AtomicParams SPSC: writer and reader run concurrently without crash", "[atomicparams][concurrent]") {
    AtomicParams<TestParams> p;
    constexpr int kIter = 50'000;

    std::atomic<bool> done{false};

    std::thread writer([&]() {
        for (int i = 0; i < kIter; ++i)
            p.publish({0.5f, i, true});
        done.store(true, std::memory_order_release);
    });

    std::thread reader([&]() {
        while (!done.load(std::memory_order_acquire))
            (void)p.read();
        (void)p.read();
    });

    writer.join();
    reader.join();

    const auto& final = p.read();
    REQUIRE(final.mode >= 0);
    REQUIRE(final.mode < kIter);
    REQUIRE(final.intensity == Catch::Approx(0.5f));
}