#pragma once
#include "AudioInput.hpp"
#include "AudioOutput.hpp"
#include <memory>

namespace echoshade {

class DSPPipeline;

struct StreamProcessorConfig {
    AudioInputConfig  input;
    AudioOutputConfig output;
};

/// Ties AudioInput -> DSPPipeline -> AudioOutput into a single real-time stream.
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

    double measuredLatencyMs() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace echoshade
