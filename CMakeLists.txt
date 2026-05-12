cmake_minimum_required(VERSION 3.28)

project(EchoShade
    VERSION 0.1.0
    DESCRIPTION "Real-time defensive audio perturbation — protect your voice from AI cloning"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(ECHOSHADE_BUILD_TESTS   "Build unit tests"              ON)
option(ECHOSHADE_BUILD_GUI     "Build GUI application"         ON)
option(ECHOSHADE_ENABLE_SIMD   "Enable SIMD optimizations"     ON)
option(ECHOSHADE_ENABLE_ASAN   "Enable AddressSanitizer"       OFF)
option(ECHOSHADE_ENABLE_TSAN   "Enable ThreadSanitizer"        OFF)

include(cmake/Platform.cmake)

include(cmake/CompilerFlags.cmake)

include(FetchContent)
include(cmake/Dependencies.cmake)

add_subdirectory(src)

if(ECHOSHADE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)

install(TARGETS echoshade_core
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/echoshade
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY platforms/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/echoshade/platforms
)
install(DIRECTORY config/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/echoshade/config
)
