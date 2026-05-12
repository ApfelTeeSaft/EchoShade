add_library(echoshade_core STATIC
    core/AudioDevice.cpp
    core/AudioInput.cpp
    core/AudioOutput.cpp
    core/DSPPipeline.cpp
    core/StreamProcessor.cpp
    perturbation/PerturbationEngine.cpp
    platforms/PlatformRegistry.cpp
    metrics/MetricsCollector.cpp
    config/Config.cpp
    file/FileProcessor.cpp
)

target_include_directories(echoshade_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(echoshade_core
    PUBLIC
        nlohmann_json::nlohmann_json
    PRIVATE
        portaudio_static
        kissfft::kissfft-float
)

if(ECHOSHADE_ENABLE_SIMD AND TARGET echoshade_simd)
    target_link_libraries(echoshade_core PRIVATE echoshade_simd)
endif()

target_compile_features(echoshade_core PUBLIC cxx_std_20)

if(ECHOSHADE_BUILD_GUI)
    add_executable(echoshade
        main.cpp
        gui/Application.cpp
    )

    target_include_directories(echoshade
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(echoshade
        PRIVATE
            echoshade_core
            imgui_sdl2_opengl3
            SDL2-static
    )

    # Platform-specific GUI dependencies
    if(ECHOSHADE_PLATFORM_LINUX)
        find_package(OpenGL REQUIRED)
        target_link_libraries(echoshade PRIVATE OpenGL::GL dl pthread)
    elseif(ECHOSHADE_PLATFORM_MACOS)
        find_library(COCOA_LIB Cocoa REQUIRED)
        target_link_libraries(echoshade PRIVATE ${COCOA_LIB} "-framework OpenGL")
    elseif(ECHOSHADE_PLATFORM_WINDOWS)
        target_link_libraries(echoshade PRIVATE opengl32)
    endif()

    install(TARGETS echoshade RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()