#include <catch2/catch_test_macros.hpp>
#include "echoshade/PlatformRegistry.hpp"
#include "echoshade/PlatformProfile.hpp"
#include <string>

using namespace echoshade;

class TestProfile final : public PlatformProfile {
public:
    std::string platformName()   const override { return "test"; }
    std::string displayName()    const override { return "Test Platform"; }
    CodecProfile primaryCodec()  const override { return {"aac_128", 128000, true, 0.0f}; }
    CodecProfile secondaryCodec()const override { return {"opus_96",  96000, true, 0.0f}; }
    float targetLufs()           const override { return -14.0f; }
    float loudnessBoostDb()      const override { return  3.0f; }
    std::vector<FrequencyBand>   frequencyEmphasis() const override { return {}; }
    void applyTuning(float, float*, float*, float*, float*) const override {}
};

TEST_CASE("PlatformRegistry register and retrieve", "[platform]") {
    auto& reg = PlatformRegistry::instance();
    reg.registerProfile(std::make_shared<TestProfile>());

    auto p = reg.get("test");
    REQUIRE(p != nullptr);
    REQUIRE(p->platformName() == "test");
    REQUIRE(p->displayName()  == "Test Platform");
}

TEST_CASE("PlatformRegistry returns null for unknown name", "[platform]") {
    auto& reg = PlatformRegistry::instance();
    REQUIRE(reg.get("nonexistent_xyz") == nullptr);
}

TEST_CASE("PlatformRegistry availableNames contains registered platform", "[platform]") {
    auto& reg = PlatformRegistry::instance();
    reg.registerProfile(std::make_shared<TestProfile>());
    auto names = reg.availableNames();
    bool found = false;
    for (const auto& n : names) if (n == "test") { found = true; break; }
    REQUIRE(found);
}
