include(CTest)
include(Catch)

add_executable(test_ringbuffer test_ringbuffer.cpp)
target_include_directories(test_ringbuffer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_ringbuffer PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(test_ringbuffer)

add_executable(test_dsp_pipeline test_dsp_pipeline.cpp)
target_include_directories(test_dsp_pipeline PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_dsp_pipeline PRIVATE Catch2::Catch2WithMain echoshade_core)
catch_discover_tests(test_dsp_pipeline)

add_executable(test_math_utils test_math_utils.cpp)
target_include_directories(test_math_utils PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_math_utils PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(test_math_utils)

add_executable(test_config test_config.cpp)
target_include_directories(test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_config PRIVATE Catch2::Catch2WithMain echoshade_core)
catch_discover_tests(test_config)

add_executable(test_platform_registry test_platform_registry.cpp)
target_include_directories(test_platform_registry PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_platform_registry PRIVATE Catch2::Catch2WithMain echoshade_core)
catch_discover_tests(test_platform_registry)
