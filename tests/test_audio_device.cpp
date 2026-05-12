#include <catch2/catch_test_macros.hpp>
#include "echoshade/AudioDevice.hpp"

using namespace echoshade;

TEST_CASE("AudioDevice::isAvailable reports PA init status", "[audio][device]") {
    const bool avail = AudioDevice::isAvailable();
    INFO("PortAudio available: " << (avail ? "yes" : "no"));
    REQUIRE_NOTHROW(AudioDevice::isAvailable());
}

TEST_CASE("AudioDevice::enumerateInputs returns vector", "[audio][device]") {
    if (!AudioDevice::isAvailable()) {
        SKIP("No PortAudio backend available in this environment.");
    }
    const auto devices = AudioDevice::enumerateInputs();
    INFO("Input devices found: " << devices.size());
    for (const auto& d : devices) {
        REQUIRE(!d.name.empty());
        REQUIRE(d.maxInputChannels > 0);
        REQUIRE(d.index >= 0);
    }
}

TEST_CASE("AudioDevice::enumerateOutputs returns vector", "[audio][device]") {
    if (!AudioDevice::isAvailable()) {
        SKIP("No PortAudio backend available in this environment.");
    }
    const auto devices = AudioDevice::enumerateOutputs();
    INFO("Output devices found: " << devices.size());
    for (const auto& d : devices) {
        REQUIRE(!d.name.empty());
        REQUIRE(d.maxOutputChannels > 0);
        REQUIRE(d.index >= 0);
    }
}

TEST_CASE("AudioDevice::defaultInput returns nullopt or valid device", "[audio][device]") {
    if (!AudioDevice::isAvailable()) {
        SKIP("No PortAudio backend available in this environment.");
    }
    const auto dev = AudioDevice::defaultInput();
    if (dev) {
        REQUIRE(dev->isDefaultInput);
        REQUIRE(!dev->name.empty());
        REQUIRE(dev->maxInputChannels > 0);
    }
}

TEST_CASE("AudioDevice::defaultOutput returns nullopt or valid device", "[audio][device]") {
    if (!AudioDevice::isAvailable()) {
        SKIP("No PortAudio backend available in this environment.");
    }
    const auto dev = AudioDevice::defaultOutput();
    if (dev) {
        REQUIRE(dev->isDefaultOutput);
        REQUIRE(!dev->name.empty());
        REQUIRE(dev->maxOutputChannels > 0);
    }
}