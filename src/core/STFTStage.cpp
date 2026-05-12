#include "echoshade/STFTStage.hpp"
#include <algorithm>

namespace echoshade {

STFTStage::STFTStage(const STFTConfig& cfg, STFTProcessor::SpectralCallback spectralCb)
    : proc_(std::make_unique<STFTProcessor>(cfg))
    , spectralCb_(std::move(spectralCb))
    , pullBuf_(cfg.fftSize * 8)
{}

void STFTStage::process(float* samples, int frameCount, int channels,
                         double sampleRate) noexcept {
    if (channels != 1) return;

    proc_->pushSamples(samples, frameCount, spectralCb_, sampleRate);

    const int avail = proc_->available();
    if (avail <= 0) return;

    const int pull = std::min(avail, frameCount);
    if (static_cast<int>(pullBuf_.size()) < pull)
        pullBuf_.resize(pull);

    const int got = proc_->pullSamples(pullBuf_.data(), pull);
    for (int i = 0; i < got; ++i)
        samples[i] = pullBuf_[i];
}

void STFTStage::reset() noexcept {
    proc_->reset();
}

} // namespace echoshade