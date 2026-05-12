#pragma once
#include "echoshade/DSPPipeline.hpp"
#include "echoshade/STFTProcessor.hpp"
#include <memory>

namespace echoshade {

class STFTStage final : public DSPStage {
public:
    explicit STFTStage(const STFTConfig& cfg,
                       STFTProcessor::SpectralCallback spectralCb = nullptr);

    void process(float* samples, int frameCount, int channels,
                 double sampleRate) noexcept override;

    void reset() noexcept override;

    const char* name() const noexcept override { return "STFTStage"; }

    STFTProcessor& processor() noexcept { return *proc_; }

    void setCallback(STFTProcessor::SpectralCallback cb) { spectralCb_ = std::move(cb); }

private:
    std::unique_ptr<STFTProcessor>      proc_;
    STFTProcessor::SpectralCallback     spectralCb_;
    std::vector<float>                  pullBuf_;
};

} // namespace echoshade