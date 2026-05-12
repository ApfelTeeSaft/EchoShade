#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "utils/RingBuffer.hpp"

using namespace echoshade;

TEST_CASE("RingBuffer basic push/pop", "[ringbuffer]") {
    RingBuffer<int> rb(16);

    REQUIRE(rb.available() == 0);
    REQUIRE(rb.capacity() == 16);

    REQUIRE(rb.push(42));
    REQUIRE(rb.push(99));
    REQUIRE(rb.available() == 2);

    int v = 0;
    REQUIRE(rb.pop(v)); REQUIRE(v == 42);
    REQUIRE(rb.pop(v)); REQUIRE(v == 99);
    REQUIRE(rb.available() == 0);
}

TEST_CASE("RingBuffer full detection", "[ringbuffer]") {
    RingBuffer<int> rb(4);
    int pushed = 0;
    while (rb.push(pushed)) ++pushed;
    REQUIRE(pushed == 3);
}

TEST_CASE("RingBuffer bulk write/read", "[ringbuffer]") {
    AudioRingBuffer rb(256);
    std::vector<float> src(100, 1.5f);
    std::size_t w = rb.write(src.data(), src.size());
    REQUIRE(w == 100);

    std::vector<float> dst(100, 0.0f);
    std::size_t r = rb.read(dst.data(), dst.size());
    REQUIRE(r == 100);
    for (float f : dst) REQUIRE(f == Catch::Approx(1.5f));
}

TEST_CASE("RingBuffer reset", "[ringbuffer]") {
    RingBuffer<int> rb(8);
    rb.push(1); rb.push(2); rb.push(3);
    REQUIRE(rb.available() == 3);
    rb.reset();
    REQUIRE(rb.available() == 0);
}

TEST_CASE("RingBuffer capacity is power of two", "[ringbuffer]") {
    RingBuffer<int> rb(13);
    REQUIRE(rb.capacity() == 16);

    RingBuffer<int> rb2(1);
    REQUIRE(rb2.capacity() == 1);

    RingBuffer<int> rb3(0);
    REQUIRE(rb3.capacity() == 1);
}
