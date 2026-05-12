#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "utils/RingBuffer.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <numeric>
#include <chrono>

using namespace echoshade;

TEST_CASE("RingBuffer SPSC: producer writes, consumer reads all values", "[ringbuffer][concurrent]") {
    constexpr int kItems = 100'000;
    RingBuffer<int> rb(4096);

    std::atomic<bool> producerDone{false};
    std::vector<int>  received;
    received.reserve(kItems);

    std::thread producer([&]() {
        for (int i = 0; i < kItems; ++i) {
            while (!rb.push(i)) { /* spin — ring full, yield to consumer */ }
        }
        producerDone.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        int v = 0;
        while (static_cast<int>(received.size()) < kItems) {
            if (rb.pop(v))
                received.push_back(v);
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(static_cast<int>(received.size()) == kItems);
    for (int i = 0; i < kItems; ++i)
        REQUIRE(received[static_cast<std::size_t>(i)] == i);
}

TEST_CASE("RingBuffer SPSC: bulk float audio streaming", "[ringbuffer][concurrent]") {
    constexpr int kBlockSize   = 512;
    constexpr int kNumBlocks   = 1000;
    constexpr std::size_t kRingCapacity = 4096;

    AudioRingBuffer rb(kRingCapacity);

    std::atomic<int> blocksWritten{0};
    std::thread producer([&]() {
        for (int b = 0; b < kNumBlocks; ++b) {
            std::vector<float> block(kBlockSize);
            for (int i = 0; i < kBlockSize; ++i)
                block[static_cast<std::size_t>(i)] = static_cast<float>(b * kBlockSize + i);
            std::size_t remaining = kBlockSize;
            const float* ptr = block.data();
            while (remaining > 0) {
                std::size_t w = rb.write(ptr, remaining);
                ptr       += w;
                remaining -= w;
            }
            blocksWritten.fetch_add(1, std::memory_order_relaxed);
        }
    });

    double totalSum = 0.0;
    const std::size_t totalSamples = static_cast<std::size_t>(kNumBlocks * kBlockSize);
    std::size_t       totalRead    = 0;
    std::thread consumer([&]() {
        std::vector<float> buf(kBlockSize);
        while (totalRead < totalSamples) {
            std::size_t got = rb.read(buf.data(), kBlockSize);
            for (std::size_t i = 0; i < got; ++i)
                totalSum += static_cast<double>(buf[i]);
            totalRead += got;
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(totalRead == totalSamples);

    const double N        = static_cast<double>(totalSamples);
    const double expected = N * (N - 1.0) / 2.0;
    REQUIRE(totalSum == Catch::Approx(expected).epsilon(1e-5));
}

TEST_CASE("RingBuffer SPSC: no data loss under sustained load", "[ringbuffer][concurrent]") {
    constexpr std::size_t kTotal = 500'000;
    RingBuffer<std::uint32_t> rb(1024);

    std::atomic<bool>      done{false};
    std::atomic<std::size_t> consumed{0};
    std::uint32_t lastValue = 0;
    bool          ordered   = true;

    std::thread prod([&]() {
        for (std::uint32_t i = 0; i < kTotal; ++i)
            while (!rb.push(i)) {}
        done.store(true, std::memory_order_release);
    });

    std::thread cons([&]() {
        std::uint32_t v = 0;
        while (consumed.load() < kTotal) {
            if (rb.pop(v)) {
                if (v != lastValue && consumed.load() > 0)
                    ordered = false;
                lastValue = v + 1;
                consumed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    prod.join();
    cons.join();

    REQUIRE(consumed.load() == kTotal);
    REQUIRE(ordered);
}

TEST_CASE("RingBuffer: available() tracks correctly under SPSC", "[ringbuffer][concurrent]") {
    RingBuffer<int> rb(128);

    for (int i = 0; i < 63; ++i) REQUIRE(rb.push(i));
    REQUIRE(rb.available() == 63);

    for (int i = 0; i < 30; ++i) {
        int v = 0;
        REQUIRE(rb.pop(v));
    }
    REQUIRE(rb.available() == 33);
}