#pragma once
#include <atomic>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>

namespace echoshade {

/// Lock-free single-producer single-consumer ring buffer of T.
/// Producer and consumer may be on different threads.
/// Capacity is always rounded up to the next power of two.
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity)
        : mask_(roundUpPow2(capacity) - 1)
        , buf_(mask_ + 1)
        , head_(0)
        , tail_(0)
    {}

    RingBuffer(const RingBuffer&)            = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    std::size_t capacity() const noexcept { return mask_ + 1; }

    // Producer thread only
    bool push(const T& item) noexcept {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t next = (h + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire))
            return false;  // full
        buf_[h] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    /// Write up to count items; returns number actually written.
    std::size_t write(const T* src, std::size_t count) noexcept {
        std::size_t written = 0;
        while (written < count && push(src[written]))
            ++written;
        return written;
    }

    // Consumer thread only
    bool pop(T& item) noexcept {
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire))
            return false;  // empty
        item = buf_[t];
        tail_.store((t + 1) & mask_, std::memory_order_release);
        return true;
    }

    /// Read up to count items; returns number actually read.
    std::size_t read(T* dst, std::size_t count) noexcept {
        std::size_t got = 0;
        while (got < count && pop(dst[got]))
            ++got;
        return got;
    }

    std::size_t available() const noexcept {
        const std::size_t h = head_.load(std::memory_order_acquire);
        const std::size_t t = tail_.load(std::memory_order_acquire);
        return (h - t) & mask_;
    }

    std::size_t free() const noexcept {
        return mask_ - available();
    }

    void reset() noexcept {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

private:
    static std::size_t roundUpPow2(std::size_t n) {
        if (n == 0) return 1;
        --n;
        n |= n >> 1; n |= n >> 2; n |= n >> 4;
        n |= n >> 8; n |= n >> 16; n |= n >> 32;
        return n + 1;
    }

    const std::size_t          mask_;
    std::vector<T>             buf_;
    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
};

/// Specialised float ring buffer for audio sample streams.
using AudioRingBuffer = RingBuffer<float>;

} // namespace echoshade
