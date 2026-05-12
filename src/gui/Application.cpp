#include "echoshade/gui/Application.hpp"
#include "echoshade/Version.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <cstdio>

namespace echoshade::gui {

struct Application::Impl {
    EchoShadeConfig& cfg;
    SDL_Window*      window  = nullptr;
    SDL_GLContext    glCtx   = nullptr;
    bool             running = false;

    explicit Impl(EchoShadeConfig& c) : cfg(c) {}
};

Application::Application(EchoShadeConfig& cfg)
    : impl_(std::make_unique<Impl>(cfg))
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    impl_->window = SDL_CreateWindow(
        "EchoShade — Voice Protection",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!impl_->window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        return;
    }

    impl_->glCtx = SDL_GL_CreateContext(impl_->window);
    SDL_GL_MakeCurrent(impl_->window, impl_->glCtx);
    SDL_GL_SetSwapInterval(1);   // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = "echoshade_ui.ini";

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(impl_->window, impl_->glCtx);
    ImGui_ImplOpenGL3_Init("#version 330");

    impl_->running = true;
}

Application::~Application() {
    if (impl_->running) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }
    if (impl_->glCtx)  SDL_GL_DeleteContext(impl_->glCtx);
    if (impl_->window) SDL_DestroyWindow(impl_->window);
    SDL_Quit();
}

int Application::run() {
    if (!impl_->running) return 1;

    while (impl_->running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) impl_->running = false;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
                impl_->running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Config"))  { /* TODO */ }
            if (ImGui::MenuItem("Load Config"))  { /* TODO */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Esc"))  impl_->running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) { /* TODO */ }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();

        // placeholder for now
        ImGui::SetNextWindowPos({0, 20}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
        ImGui::Begin("EchoShade", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::TextColored({0.4f, 0.9f, 0.4f, 1.0f}, "EchoShade v%s", VERSION_STRING);
        ImGui::TextDisabled("Real-time defensive voice protection");
        ImGui::Separator();
        ImGui::TextWrapped(
            "GUI panels are implemented in future "
            "The core DSP and perturbation engine are not here yet."
        );
        ImGui::Separator();
        ImGui::Text("Platform : %s", impl_->cfg.platformName.c_str());
        ImGui::Text("Intensity: %.2f", impl_->cfg.perturbation.intensity);
        ImGui::Text("Streaming: %s",   impl_->cfg.streamingEnabled ? "ON" : "OFF");

        ImGui::End();

        ImGui::Render();
        int w, h;
        SDL_GL_GetDrawableSize(impl_->window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(impl_->window);
    }
    return 0;
}

} // namespace echoshade::gui
