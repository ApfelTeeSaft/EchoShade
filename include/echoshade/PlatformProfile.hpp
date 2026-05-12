#pragma once
#include <string>
#include <vector>

namespace echoshade {

/// Frequency band emphasis entry for platform-specific perturbation shaping.
struct FrequencyBand {
    float lowHz;
    float highHz;
    float gainFactor;   // multiplier on perturbation amplitude within this band
};

/// Codec survivability parameters — tuned per platform.
struct CodecProfile {
    std::string name;           // e.g. "aac_128", "opus_160"
    int         bitrate         = 128000;
    bool        isMDCT          = true;  // AAC/MP3 vs. LPC-based (SILK/Opus narrow)
    float       perturbBoostDb  = 0.0f;  // pre-compensation for codec attenuation
};

/// Complete platform profile defining how perturbations are shaped and tuned.
class PlatformProfile {
public:
    virtual ~PlatformProfile() = default;

    virtual std::string         platformName()    const = 0;
    virtual std::string         displayName()     const = 0;
    virtual CodecProfile        primaryCodec()    const = 0;
    virtual CodecProfile        secondaryCodec()  const = 0;
    virtual float               targetLufs()      const = 0;  // loudness normalization target
    virtual float               loudnessBoostDb() const = 0;  // pre-compensation
    virtual std::vector<FrequencyBand> frequencyEmphasis() const = 0;

    /// Apply profile-specific tuning to a PerturbationConfig's internal parameters.
    /// Called by PerturbationEngine when the profile is set.
    virtual void applyTuning(float intensity,
                              float* phaseNoiseScale,
                              float* magnitudeNoiseScale,
                              float* formantDriftHz,
                              float* harmonicAmplitude) const = 0;
};

} // namespace echoshade
