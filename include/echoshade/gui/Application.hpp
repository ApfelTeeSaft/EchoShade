#pragma once
#include "../Config.hpp"
#include <memory>

namespace echoshade::gui {

/// Top-level GUI application. Owns the SDL2 window, ImGui context,
/// and all panel widgets. Drives the main event loop.
class Application {
public:
    explicit Application(EchoShadeConfig& cfg);
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace echoshade::gui
