#include "echoshade/Config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

namespace echoshade {

using json = nlohmann::json;

EchoShadeConfig EchoShadeConfig::defaults() {
    EchoShadeConfig cfg;
    cfg.platformName             = "twitch";
    cfg.audioInput.channels      = 1;
    cfg.audioInput.sampleRate    = 48000.0;
    cfg.audioInput.framesPerBuffer = 512;
    cfg.audioOutput.channels     = 1;
    cfg.audioOutput.sampleRate   = 48000.0;
    cfg.audioOutput.framesPerBuffer = 512;
    cfg.perturbation.intensity   = 0.5f;
    cfg.streamingEnabled         = false;
    return cfg;
}

EchoShadeConfig EchoShadeConfig::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open config: " + path);
    json j;
    f >> j;

    EchoShadeConfig cfg = defaults();
    if (j.contains("platform"))   cfg.platformName = j["platform"].get<std::string>();
    if (j.contains("intensity"))   cfg.perturbation.intensity = j["intensity"].get<float>();
    if (j.contains("streaming"))   cfg.streamingEnabled       = j["streaming"].get<bool>();
    if (j.contains("sampleRate")) {
        double sr = j["sampleRate"].get<double>();
        cfg.audioInput.sampleRate  = sr;
        cfg.audioOutput.sampleRate = sr;
    }
    return cfg;
}

void EchoShadeConfig::saveToFile(const std::string& path) const {
    json j;
    j["platform"]   = platformName;
    j["intensity"]  = perturbation.intensity;
    j["streaming"]  = streamingEnabled;
    j["sampleRate"] = audioInput.sampleRate;
    j["enableEmbeddingDisruptor"]    = perturbation.enableEmbeddingDisruptor;
    j["enableFormantDestabilizer"]   = perturbation.enableFormantDestabilizer;
    j["enableHarmonicPerturber"]     = perturbation.enableHarmonicPerturber;
    j["enablePhasePerturber"]        = perturbation.enablePhasePerturber;
    j["enableAdaptiveNoise"]         = perturbation.enableAdaptiveNoise;
    j["enableTemporalInconsistency"] = perturbation.enableTemporalInconsistency;

    std::ofstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot write config: " + path);
    f << j.dump(4) << "\n";
}

} // namespace echoshade
