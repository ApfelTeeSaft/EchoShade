#include "echoshade/DSPPipeline.hpp"

namespace echoshade {

void DSPPipeline::addStage(std::shared_ptr<DSPStage> stage) {
    stages_.push_back(std::move(stage));
}

void DSPPipeline::clearStages() {
    stages_.clear();
}

void DSPPipeline::process(float* samples, int frameCount, int channels,
                          double sampleRate) noexcept {
    for (auto& stage : stages_)
        stage->process(samples, frameCount, channels, sampleRate);
}

void DSPPipeline::resetAll() noexcept {
    for (auto& stage : stages_)
        stage->reset();
}

} // namespace echoshade
