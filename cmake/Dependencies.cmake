include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG        131.1.0
    GIT_SHALLOW    TRUE
)
set(KISSFFT_TEST        OFF CACHE BOOL "" FORCE)
set(KISSFFT_TOOLS       OFF CACHE BOOL "" FORCE)
set(KISSFFT_STATIC      ON  CACHE BOOL "" FORCE)
set(KISSFFT_USE_ALLOCA  OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(kissfft)

FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG        v19.7.0
    GIT_SHALLOW    TRUE
)
set(PA_BUILD_SHARED_LIBS     OFF CACHE BOOL "" FORCE)
set(PA_BUILD_STATIC_LIBS     ON  CACHE BOOL "" FORCE)
set(PA_ENABLE_DEBUG_OUTPUT   OFF CACHE BOOL "" FORCE)
set(PA_BUILD_EXAMPLES        OFF CACHE BOOL "" FORCE)
set(PA_BUILD_TESTS           OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(portaudio)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests       OFF CACHE BOOL "" FORCE)
set(JSON_Install          OFF CACHE BOOL "" FORCE)
set(JSON_MultipleHeaders  OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE
)
set(CATCH_BUILD_TESTING     OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_DOCS      OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_HELPERS   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Catch2)

if(ECHOSHADE_BUILD_GUI)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-2.30.12
        GIT_SHALLOW    TRUE
    )
    set(SDL_SHARED             OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC             ON  CACHE BOOL "" FORCE)
    set(SDL_TEST               OFF CACHE BOOL "" FORCE)
    set(SDL_AUDIO              ON  CACHE BOOL "" FORCE)
    set(SDL_VIDEO              ON  CACHE BOOL "" FORCE)
    set(SDL_RENDER             ON  CACHE BOOL "" FORCE)
    set(SDL_OPENGL             ON  CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL   ON  CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL2)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.5
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(imgui)

    FetchContent_Declare(
        implot
        GIT_REPOSITORY https://github.com/epezent/implot.git
        GIT_TAG        v0.17
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(implot)

    find_package(OpenGL REQUIRED)

    add_library(imgui_sdl2_opengl3 STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${implot_SOURCE_DIR}/implot.cpp
        ${implot_SOURCE_DIR}/implot_items.cpp
    )
    target_include_directories(imgui_sdl2_opengl3
        PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
            ${implot_SOURCE_DIR}
    )
    target_link_libraries(imgui_sdl2_opengl3 PUBLIC SDL2-static OpenGL::GL)
    set_target_properties(imgui_sdl2_opengl3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif() # ECHOSHADE_BUILD_GUI

message(STATUS "EchoShade: Dependencies configured via FetchContent")
