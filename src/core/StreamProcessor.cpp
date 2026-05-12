#include "echoshade/StreamProcessor.hpp"
#include "echoshade/DSPPipeline.hpp"
#include "../utils/RingBuffer.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>
#include <cassert>

namespace echoshade {

struct StreamProcessor::Impl {
    AudioInput   input;
    AudioOutput  output;

    std::shared_ptr<DSPPipeline> pipeline;

    // Sized at construction time; input callback processes in-place here.
    std::vector<float> scratch;          // [framesPerBuffer * channels]

    AudioRingBuffer outRing;

    std::atomic<bool>          running        {false};
    std::atomic<double>        dspMs          {0.0};
    std::atomic<std::uint64_t> underrunCount  {0};
    std::atomic<std::uint64_t> overrunCount   {0};
    std::atomic<float>         inPeak         {0.0f};
    std::atomic<float>         outPeak        {0.0f};

    const double sampleRate;
    const int    channels;
    const int    framesPerBuffer;

    Impl(const StreamProcessorConfig& cfg)
        : input (cfg.input)
        , output(cfg.output)
        , scratch(static_cast<std::size_t>(cfg.input.framesPerBuffer * cfg.input.channels), 0.0f)
        , outRing(static_cast<std::size_t>(
              cfg.input.framesPerBuffer * cfg.input.channels
              * cfg.ringDepthBuffers))
        , sampleRate    (cfg.input.sampleRate)
        , channels      (cfg.input.channels)
        , framesPerBuffer(cfg.input.framesPerBuffer)
    {}
};

static float peakOf(const float* buf, int n) noexcept {
    float pk = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float v = buf[i] < 0.0f ? -buf[i] : buf[i];
        if (v > pk) pk = v;
    }
    return pk;
}

StreamProcessor::StreamProcessor(const StreamProcessorConfig& cfg)
    : impl_(std::make_unique<Impl>(cfg))
{
    impl_->input.setCallback([this](const float* samples, int frameCount, int ch) {
        Impl& im = *impl_;
        const int n = frameCount * ch;

        std::memcpy(im.scratch.data(), samples, static_cast<std::size_t>(n) * sizeof(float));

        im.inPeak.store(peakOf(im.scratch.data(), n), std::memory_order_relaxed);

        if (im.pipeline) {
            const auto t0 = std::chrono::steady_clock::now();
            im.pipeline->process(im.scratch.data(), frameCount, ch, im.sampleRate);
            const auto t1 = std::chrono::steady_clock::now();
            im.dspMs.store(
                std::chrono::duration<double, std::milli>(t1 - t0).count(),
                std::memory_order_relaxed);
        }

        const std::size_t written =
            im.outRing.write(im.scratch.data(), static_cast<std::size_t>(n));
        if (written < static_cast<std::size_t>(n))
            im.overrunCount.fetch_add(1, std::memory_order_relaxed);
    });

    impl_->output.setCallback([this](float* out, int frameCount, int ch) {
        Impl& im = *impl_;
        const std::size_t need = static_cast<std::size_t>(frameCount * ch);

        const std::size_t got = im.outRing.read(out, need);
        if (got < need) {
            // Silence-fill any shortfall (underrun).
            std::memset(out + got, 0, (need - got) * sizeof(float));
            im.underrunCount.fetch_add(1, std::memory_order_relaxed);
        }

        im.outPeak.store(peakOf(out, frameCount * ch), std::memory_order_relaxed);
    });
}

StreamProcessor::~StreamProcessor() {
    stop();
}

void StreamProcessor::setPipeline(std::shared_ptr<DSPPipeline> pipeline) {
    assert(!impl_->running.load(std::memory_order_relaxed)
           && "setPipeline() must be called while the stream is stopped.");
    impl_->pipeline = std::move(pipeline);
}

bool StreamProcessor::start() {
    if (impl_->running.load(std::memory_order_relaxed)) return true;

    impl_->underrunCount.store(0, std::memory_order_relaxed);
    impl_->overrunCount .store(0, std::memory_order_relaxed);
    impl_->inPeak       .store(0.0f, std::memory_order_relaxed);
    impl_->outPeak      .store(0.0f, std::memory_order_relaxed);
    impl_->dspMs        .store(0.0,  std::memory_order_relaxed);
    impl_->outRing.reset();

    if (impl_->pipeline)
        impl_->pipeline->resetAll();

    const bool ok = impl_->input.start() && impl_->output.start();
    impl_->running.store(ok, std::memory_order_release);
    return ok;
}

void StreamProcessor::stop() {
    if (!impl_->running.load(std::memory_order_relaxed)) return;
    impl_->input.stop();
    impl_->output.stop();
    impl_->running.store(false, std::memory_order_release);
}

bool StreamProcessor::isRunning() const {
    return impl_->running.load(std::memory_order_relaxed);
}

double StreamProcessor::dspProcessingMs() const {
    return impl_->dspMs.load(std::memory_order_relaxed);
}

std::uint64_t StreamProcessor::underruns() const {
    return impl_->underrunCount.load(std::memory_order_relaxed);
}

std::uint64_t StreamProcessor::overruns() const {
    return impl_->overrunCount.load(std::memory_order_relaxed);
}

float StreamProcessor::inputPeakLinear() const {
    return impl_->inPeak.load(std::memory_order_relaxed);
}

float StreamProcessor::outputPeakLinear() const {
    return impl_->outPeak.load(std::memory_order_relaxed);
}

} // namespace echoshade
