#include "echoshade/MetricsCollector.hpp"

namespace echoshade {

void MetricsCollector::recordLatency(float ms) noexcept {
    slots_[writeSlot_.load(std::memory_order_relaxed)].latencyMs.store(ms, std::memory_order_relaxed);
}

void MetricsCollector::recordCPU(float percent) noexcept {
    slots_[writeSlot_.load(std::memory_order_relaxed)].cpuPercent.store(percent, std::memory_order_relaxed);
}

void MetricsCollector::recordSNR(float snrDb) noexcept {
    slots_[writeSlot_.load(std::memory_order_relaxed)].snrDb.store(snrDb, std::memory_order_relaxed);
}

void MetricsCollector::recordPeaks(float inDb, float outDb) noexcept {
    int w = writeSlot_.load(std::memory_order_relaxed);
    slots_[w].inputPeak.store(inDb,  std::memory_order_relaxed);
    slots_[w].outputPeak.store(outDb, std::memory_order_relaxed);
}

void MetricsCollector::incrementUnderruns() noexcept {
    slots_[writeSlot_.load(std::memory_order_relaxed)].underruns.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::incrementOverruns() noexcept {
    slots_[writeSlot_.load(std::memory_order_relaxed)].overruns.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::incrementFrameCount(std::uint64_t n) noexcept {
    slots_[writeSlot_.load(std::memory_order_relaxed)].frames.fetch_add(n, std::memory_order_relaxed);
    // Promote the write slot so the reader sees fresh data.
    writeSlot_.store(1 - writeSlot_.load(std::memory_order_relaxed), std::memory_order_release);
}

AudioMetrics MetricsCollector::snapshot() const noexcept {
    const int r = 1 - writeSlot_.load(std::memory_order_acquire);
    const auto& s = slots_[r];
    AudioMetrics m;
    m.latencyMs   = s.latencyMs.load(std::memory_order_relaxed);
    m.cpuPercent  = s.cpuPercent.load(std::memory_order_relaxed);
    m.perturbSNRdB = s.snrDb.load(std::memory_order_relaxed);
    m.inputPeakDb  = s.inputPeak.load(std::memory_order_relaxed);
    m.outputPeakDb = s.outputPeak.load(std::memory_order_relaxed);
    m.underruns    = s.underruns.load(std::memory_order_relaxed);
    m.overruns     = s.overruns.load(std::memory_order_relaxed);
    m.frameCount   = s.frames.load(std::memory_order_relaxed);
    return m;
}

} // namespace echoshade
