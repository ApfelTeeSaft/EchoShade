#pragma once
#include "DSPPipeline.hpp"
#include <memory>
#include <string>

namespace echoshade {

class PlatformProfile;

struct PerturbationConfig {
    float intensity        = 0.5f;  // 0.0 = off, 1.0 = maximum
    bool  enableEmbeddingDisruptor      = true;
    bool  enableFormantDestabilizer     = true;
    bool  enableHarmonicPerturber       = true;
    bool  enablePhasePerturber          = true;
    bool  enableAdaptiveNoise           = true;
    bool  enableTemporalInconsistency   = true;
};

/// Orchestrates all perturbation sub-modules as a single DSPStage.
/// Configured by a PlatformProfile that tunes parameters for the target platform.
class PerturbationEngine final : public DSPStage {
public:
    PerturbationEngine(const PerturbationConfig& cfg,
                       std::shared_ptr<PlatformProfile> profile);
    ~PerturbationEngine() override;

    void process(float* samples, int frameCount, int channels,
                 double sampleRate) noexcept override;
    void reset() noexcept override;
    const char* name() const noexcept override { return "PerturbationEngine"; }

    void setIntensity(float intensity);
    void setConfig(const PerturbationConfig& cfg);
    void setPlatformProfile(std::shared_ptr<PlatformProfile> profile);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace echoshade
