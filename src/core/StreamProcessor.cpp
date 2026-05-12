#include "echoshade/StreamProcessor.hpp"
#include "echoshade/DSPPipeline.hpp"
#include "../utils/RingBuffer.hpp"
#include <atomic>
#include <chrono>

namespace echoshade {

struct StreamProcessor::Impl {
    AudioInput                    input;
    AudioOutput                   output;
    std::shared_ptr<DSPPipeline>  pipeline;
    AudioRingBuffer               inRing;
    AudioRingBuffer               outRing;
    std::atomic<bool>             running{false};
    std::atomic<double>           latencyMs{0.0};
    const int                     framesPerBuffer;
    const int                     channels;
    const double                  sampleRate;

    Impl(const StreamProcessorConfig& cfg)
        : input (cfg.input)
        , output(cfg.output)
        , inRing (static_cast<std::size_t>(cfg.input.framesPerBuffer  * cfg.input.channels  * 8))
        , outRing(static_cast<std::size_t>(cfg.output.framesPerBuffer * cfg.output.channels * 8))
        , framesPerBuffer(cfg.input.framesPerBuffer)
        , channels(cfg.input.channels)
        , sampleRate(cfg.input.sampleRate)
    {}
};

StreamProcessor::StreamProcessor(const StreamProcessorConfig& cfg)
    : impl_(std::make_unique<Impl>(cfg))
{
    impl_->input.setCallback([this](const float* samples, int frameCount, int ch) {
        impl_->inRing.write(samples, static_cast<std::size_t>(frameCount * ch));
    });

    impl_->output.setCallback([this](float* out, int frameCount, int ch) {
        auto t0 = std::chrono::steady_clock::now();

        const std::size_t need = static_cast<std::size_t>(frameCount * ch);
        
        std::size_t got = impl_->outRing.read(out, need);

        // If outRing is empty, pull from inRing, process, and write directly.
        if (got < need) {
            const std::size_t remaining = need - got;
            std::size_t pulled = impl_->inRing.read(out + got, remaining);
            // Zero-fill any gap.
            if (pulled < remaining)
                std::fill(out + got + pulled, out + need, 0.0f);

            // Run DSP in-place on newly pulled samples.
            if (impl_->pipeline)
                impl_->pipeline->process(out + got,
                                         static_cast<int>((pulled ? pulled : remaining) / ch),
                                         ch, impl_->sampleRate);
        }

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        impl_->latencyMs.store(ms, std::memory_order_relaxed);
    });
}

StreamProcessor::~StreamProcessor() {
    stop();
}

void StreamProcessor::setPipeline(std::shared_ptr<DSPPipeline> pipeline) {
    impl_->pipeline = std::move(pipeline);
}

bool StreamProcessor::start() {
    if (impl_->running.load()) return true;
    bool ok = impl_->input.start() && impl_->output.start();
    impl_->running.store(ok);
    return ok;
}

void StreamProcessor::stop() {
    if (!impl_->running.load()) return;
    impl_->input.stop();
    impl_->output.stop();
    impl_->running.store(false);
}

bool StreamProcessor::isRunning() const {
    return impl_->running.load(std::memory_order_relaxed);
}

double StreamProcessor::measuredLatencyMs() const {
    return impl_->latencyMs.load(std::memory_order_relaxed);
}

} // namespace echoshade
