#pragma once
#include <vector>
#include <memory>
#include <cstdint>

namespace echoshade {

/// Abstract base for a single DSP processing stage.
/// Concrete stages are added to a DSPPipeline and called in order.
/// The process() method must be allocation-free and lock-free.
class DSPStage {
public:
    virtual ~DSPStage() = default;

    /// Process in-place. samples is interleaved [L, R, L, R, …] or mono.
    /// Called on the real-time audio thread.
    virtual void process(float* samples, int frameCount, int channels,
                         double sampleRate) noexcept = 0;

    /// Called from the main thread to reset internal state.
    virtual void reset() noexcept {}

    virtual const char* name() const noexcept = 0;
};

/// Ordered chain of DSPStage processors.
/// Stages are set from the main thread; processing happens on the audio thread.
class DSPPipeline {
public:
    DSPPipeline() = default;
    ~DSPPipeline() = default;

    DSPPipeline(const DSPPipeline&)            = delete;
    DSPPipeline& operator=(const DSPPipeline&) = delete;

    /// Main-thread: add a stage to the end of the chain.
    void addStage(std::shared_ptr<DSPStage> stage);

    /// Main-thread: remove all stages.
    void clearStages();

    /// Audio-thread: run all stages in order.
    void process(float* samples, int frameCount, int channels,
                 double sampleRate) noexcept;

    /// Main-thread: reset all stages (e.g. on stream restart).
    void resetAll() noexcept;

private:
    // Simple vector protected by the fact that modification only happens
    // while the stream is stopped. will try to make this better soon
    std::vector<std::shared_ptr<DSPStage>> stages_;
};

} // namespace echoshade
