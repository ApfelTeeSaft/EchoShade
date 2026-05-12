#include "echoshade/PlatformRegistry.hpp"
#include <stdexcept>

namespace echoshade {

PlatformRegistry& PlatformRegistry::instance() {
    static PlatformRegistry reg;
    return reg;
}

void PlatformRegistry::registerProfile(std::shared_ptr<PlatformProfile> profile) {
    if (!profile) return;
    profiles_[profile->platformName()] = std::move(profile);
}

std::shared_ptr<PlatformProfile> PlatformRegistry::get(const std::string& name) const {
    auto it = profiles_.find(name);
    return it != profiles_.end() ? it->second : nullptr;
}

std::vector<std::string> PlatformRegistry::availableNames() const {
    std::vector<std::string> names;
    names.reserve(profiles_.size());
    for (const auto& [k, _] : profiles_)
        names.push_back(k);
    return names;
}

} // namespace echoshade
