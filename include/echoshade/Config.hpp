#pragma once
#include "PerturbationEngine.hpp"
#include "AudioInput.hpp"
#include "AudioOutput.hpp"
#include <string>

namespace echoshade {

struct EchoShadeConfig {
    std::string        platformName     = "twitch";
    AudioInputConfig   audioInput;
    AudioOutputConfig  audioOutput;
    PerturbationConfig perturbation;
    bool               streamingEnabled = false;

    static EchoShadeConfig loadFromFile(const std::string& path);
    void                   saveToFile(const std::string& path) const;
    static EchoShadeConfig defaults();
};

} // namespace echoshade
