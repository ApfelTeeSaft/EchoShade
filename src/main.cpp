#include "echoshade/Config.hpp"
#include "echoshade/Version.hpp"
#include "echoshade/gui/Application.hpp"
#include <cstdio>
#include <string>
#include <vector>

static void printUsage(const char* argv0) {
    std::printf(
        "EchoShade v%s — Real-time voice protection\n"
        "\n"
        "Usage:\n"
        "  %s [options]\n"
        "\n"
        "Options:\n"
        "  --platform <name>     Target platform (twitch, discord, youtube) [default: twitch]\n"
        "  --intensity <0-1>     Perturbation intensity [default: 0.5]\n"
        "  --config <file>       Load config from JSON file\n"
        "  --no-gui              Run headless (CLI only, future use)\n"
        "  --help                Show this message\n"
        "\n",
        echoshade::VERSION_STRING, argv0
    );
}

int main(int argc, char* argv[]) {
    echoshade::EchoShadeConfig cfg = echoshade::EchoShadeConfig::defaults();
    bool noGui = false;
    std::string configFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--no-gui") {
            noGui = true;
        } else if (arg == "--platform" && i + 1 < argc) {
            cfg.platformName = argv[++i];
        } else if (arg == "--intensity" && i + 1 < argc) {
            cfg.perturbation.intensity = std::stof(argv[++i]);
        } else if (arg == "--config" && i + 1 < argc) {
            configFile = argv[++i];
        }
    }

    if (!configFile.empty()) {
        try {
            cfg = echoshade::EchoShadeConfig::loadFromFile(configFile);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "Config load error: %s\n", e.what());
            return 1;
        }
    }

    if (noGui) {
        std::printf("Headless mode not yet implemented. Use GUI.\n");
        return 0;
    }

    echoshade::gui::Application app(cfg);
    return app.run();
}
