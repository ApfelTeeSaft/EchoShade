#pragma once
#include <atomic>
#include <array>
#include <cstring>

namespace echoshade {

/// Double-buffered parameter block for lock-free main->audio-thread communication.
///
/// Main thread writes to the inactive buffer and atomically promotes it.
/// Audio thread reads from the last-promoted buffer.
///
/// T must be trivially copyable and small enough that memcpy is safe.
template <typename T>
class AtomicParams {
public:
    explicit AtomicParams(const T& initial = T{}) {
        bufs_[0] = initial;
        bufs_[1] = initial;
        readIdx_.store(0, std::memory_order_relaxed);
    }

    void publish(const T& params) noexcept {
        const int w = 1 - readIdx_.load(std::memory_order_relaxed);
        bufs_[w] = params;
        readIdx_.store(w, std::memory_order_release);
    }

    const T& read() const noexcept {
        return bufs_[readIdx_.load(std::memory_order_acquire)];
    }

private:
    std::array<T, 2>     bufs_;
    std::atomic<int>     readIdx_;
};

} // namespace echoshade