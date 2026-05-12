#include "echoshade/PerturbationEngine.hpp"
#include "echoshade/PlatformProfile.hpp"
#include "../utils/MathUtils.hpp"
#include <cstring>

namespace echoshade {

// Stubbed for future completion
struct PerturbationEngine::Impl {
    PerturbationConfig              cfg;
    std::shared_ptr<PlatformProfile> profile;
};

PerturbationEngine::PerturbationEngine(const PerturbationConfig& cfg,
                                       std::shared_ptr<PlatformProfile> profile)
    : impl_(std::make_unique<Impl>())
{
    impl_->cfg     = cfg;
    impl_->profile = std::move(profile);
}

PerturbationEngine::~PerturbationEngine() = default;

void PerturbationEngine::process(float* /*samples*/, int /*frameCount*/,
                                  int /*channels*/, double /*sampleRate*/) noexcept {
    // TODO: implement the full adversarial perturbation pipeline.
}

void PerturbationEngine::reset() noexcept {}

void PerturbationEngine::setIntensity(float intensity) {
    impl_->cfg.intensity = math::clamp(intensity, 0.0f, 1.0f);
}

void PerturbationEngine::setConfig(const PerturbationConfig& cfg) {
    impl_->cfg = cfg;
}

void PerturbationEngine::setPlatformProfile(std::shared_ptr<PlatformProfile> profile) {
    impl_->profile = std::move(profile);
}

} // namespace echoshade
