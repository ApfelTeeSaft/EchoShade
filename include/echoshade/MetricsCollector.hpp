#pragma once
#include <atomic>
#include <cstdint>
#include <array>

namespace echoshade {

struct AudioMetrics {
    float latencyMs         = 0.0f;
    float cpuPercent        = 0.0f;
    float perturbSNRdB      = 0.0f;
    float inputPeakDb       = -96.0f;
    float outputPeakDb      = -96.0f;
    std::uint64_t underruns = 0;
    std::uint64_t overruns  = 0;
    std::uint64_t frameCount = 0;
};

class MetricsCollector {
public:
    MetricsCollector() = default;

    // Audio thread writes:
    void recordLatency(float ms) noexcept;
    void recordCPU(float percent) noexcept;
    void recordSNR(float snrDb) noexcept;
    void recordPeaks(float inDb, float outDb) noexcept;
    void incrementUnderruns() noexcept;
    void incrementOverruns() noexcept;
    void incrementFrameCount(std::uint64_t n) noexcept;

    // Main/GUI thread reads:
    AudioMetrics snapshot() const noexcept;

private:
    // Double-buffer: audio thread alternates write slot; reader always gets last complete slot.
    struct alignas(64) Slot {
        std::atomic<float>         latencyMs   {0.0f};
        std::atomic<float>         cpuPercent  {0.0f};
        std::atomic<float>         snrDb       {0.0f};
        std::atomic<float>         inputPeak   {-96.0f};
        std::atomic<float>         outputPeak  {-96.0f};
        std::atomic<std::uint64_t> underruns   {0};
        std::atomic<std::uint64_t> overruns    {0};
        std::atomic<std::uint64_t> frames      {0};
    };

    std::array<Slot, 2>      slots_;
    std::atomic<int>         writeSlot_ {0};
};

} // namespace echoshade
