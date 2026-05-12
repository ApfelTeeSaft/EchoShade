#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "echoshade/Config.hpp"
#include <filesystem>
#include <cstdio>

using namespace echoshade;
using Catch::Approx;

TEST_CASE("EchoShadeConfig defaults are valid", "[config]") {
    auto cfg = EchoShadeConfig::defaults();
    REQUIRE(cfg.platformName == "twitch");
    REQUIRE(cfg.perturbation.intensity == Approx(0.5f));
    REQUIRE(cfg.audioInput.sampleRate  == Approx(48000.0));
    REQUIRE(cfg.streamingEnabled       == false);
}

TEST_CASE("EchoShadeConfig save and reload round-trip", "[config]") {
    auto cfg = EchoShadeConfig::defaults();
    cfg.platformName           = "discord";
    cfg.perturbation.intensity = 0.75f;
    cfg.streamingEnabled       = true;

    const std::string path = "/tmp/echoshade_test_config.json";
    cfg.saveToFile(path);

    auto loaded = EchoShadeConfig::loadFromFile(path);
    REQUIRE(loaded.platformName           == "discord");
    REQUIRE(loaded.perturbation.intensity == Approx(0.75f));
    REQUIRE(loaded.streamingEnabled       == true);

    std::filesystem::remove(path);
}

TEST_CASE("EchoShadeConfig loadFromFile throws on missing file", "[config]") {
    REQUIRE_THROWS_AS(
        EchoShadeConfig::loadFromFile("/nonexistent/path/config.json"),
        std::runtime_error
    );
}
