include(CTest)
include(Catch)

function(echoshade_test name src)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain echoshade_core)
    catch_discover_tests(${name})
endfunction()

echoshade_test(test_ringbuffer         test_ringbuffer.cpp)
echoshade_test(test_dsp_pipeline       test_dsp_pipeline.cpp)
echoshade_test(test_math_utils         test_math_utils.cpp)
echoshade_test(test_config             test_config.cpp)
echoshade_test(test_platform_registry  test_platform_registry.cpp)
echoshade_test(test_ringbuffer_concurrent  test_ringbuffer_concurrent.cpp)
echoshade_test(test_latency_bench         test_latency_bench.cpp)
echoshade_test(test_atomic_params         test_atomic_params.cpp)
echoshade_test(test_audio_device          test_audio_device.cpp)