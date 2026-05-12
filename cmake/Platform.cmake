if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(ECHOSHADE_PLATFORM_LINUX TRUE)
    add_compile_definitions(ECHOSHADE_PLATFORM_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(ECHOSHADE_PLATFORM_MACOS TRUE)
    add_compile_definitions(ECHOSHADE_PLATFORM_MACOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ECHOSHADE_PLATFORM_WINDOWS TRUE)
    add_compile_definitions(ECHOSHADE_PLATFORM_WINDOWS)
    add_compile_definitions(_WIN32_WINNT=0x0A00)   # Windows 10+
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    message(WARNING "Unknown platform: ${CMAKE_SYSTEM_NAME} — some features may not work.")
endif()

# SIMD capability detection
if(ECHOSHADE_ENABLE_SIMD)
    include(cmake/SIMD.cmake)
endif()
