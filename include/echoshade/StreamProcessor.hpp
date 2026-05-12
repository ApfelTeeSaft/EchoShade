#pragma once
#include "AudioInput.hpp"
#include "AudioOutput.hpp"
#include <memory>
#include <cstdint>

namespace echoshade {

class DSPPipeline;

struct StreamProcessorConfig {
    AudioInputConfig  input;
    AudioOutputConfig output;
    // How many output-buffer lengths to keep in the outRing.
    // Higher = more jitter tolerance, more latency.
    int ringDepthBuffers = 4;
};

/// Real-time audio processor: AudioInput -> DSPPipeline -> AudioOutput.
///
/// Design
///
/// The INPUT callback runs the DSPPipeline in-place on each incoming frame
/// and writes the result to an outRing buffer.  The OUTPUT callback reads
/// exclusively from outRing.  Both callbacks are allocation-free and lock-free.
///
/// DSPPipeline swap
///
/// setPipeline() may only be called while the stream is STOPPED.
class StreamProcessor {
public:
    explicit StreamProcessor(const StreamProcessorConfig& cfg);
    ~StreamProcessor();

    StreamProcessor(const StreamProcessor&)            = delete;
    StreamProcessor& operator=(const StreamProcessor&) = delete;

    void setPipeline(std::shared_ptr<DSPPipeline> pipeline);

    bool start();
    void stop();
    bool isRunning() const;

    /// Wall-clock time spent running DSPPipeline inside the last input callback.
    double dspProcessingMs()   const;

    /// Number of output underruns since last start().
    std::uint64_t underruns()  const;

    /// Number of input overruns (outRing full) since last start().
    std::uint64_t overruns()   const;

    /// Measured peak amplitude [0,1] of last input frame.
    float inputPeakLinear()    const;

    /// Measured peak amplitude [0,1] of last output frame.
    float outputPeakLinear()   const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace echoshade
