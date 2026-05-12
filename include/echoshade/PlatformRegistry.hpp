#pragma once
#include "PlatformProfile.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace echoshade {

/// Singleton registry of all known platform profiles.
/// Platforms register themselves at static-init time via PlatformRegistrar.
class PlatformRegistry {
public:
    static PlatformRegistry& instance();

    void registerProfile(std::shared_ptr<PlatformProfile> profile);
    std::shared_ptr<PlatformProfile> get(const std::string& name) const;
    std::vector<std::string>         availableNames() const;

private:
    PlatformRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<PlatformProfile>> profiles_;
};

/// RAII helper for static-init registration.
struct PlatformRegistrar {
    explicit PlatformRegistrar(std::shared_ptr<PlatformProfile> p) {
        PlatformRegistry::instance().registerProfile(std::move(p));
    }
};

} // namespace echoshade
