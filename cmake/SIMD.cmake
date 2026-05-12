include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

set(ECHOSHADE_HAS_AVX2  FALSE)
set(ECHOSHADE_HAS_SSE42 FALSE)

if(NOT MSVC)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "-mavx2;-mfma")
    check_cxx_source_compiles("
        #include <immintrin.h>
        int main() { __m256i v = _mm256_setzero_si256(); (void)v; return 0; }
    " _have_avx2)
    cmake_pop_check_state()

    if(_have_avx2)
        set(ECHOSHADE_HAS_AVX2 TRUE)
        add_compile_definitions(ECHOSHADE_HAS_AVX2)
        message(STATUS "EchoShade: AVX2 SIMD enabled")
    else()
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_FLAGS "-msse4.2")
        check_cxx_source_compiles("
            #include <nmmintrin.h>
            int main() { return (int)_mm_crc32_u8(0, 0); }
        " _have_sse42)
        cmake_pop_check_state()

        if(_have_sse42)
            set(ECHOSHADE_HAS_SSE42 TRUE)
            add_compile_definitions(ECHOSHADE_HAS_SSE42)
            message(STATUS "EchoShade: SSE4.2 SIMD enabled")
        else()
            message(STATUS "EchoShade: No SIMD intrinsics available — using scalar fallback")
        endif()
    endif()
elseif(MSVC)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "/arch:AVX2")
    check_cxx_source_compiles("
        #include <immintrin.h>
        int main() { __m256i v = _mm256_setzero_si256(); (void)v; return 0; }
    " _have_avx2_msvc)
    cmake_pop_check_state()

    if(_have_avx2_msvc)
        set(ECHOSHADE_HAS_AVX2 TRUE)
        add_compile_definitions(ECHOSHADE_HAS_AVX2)
        message(STATUS "EchoShade: AVX2 SIMD enabled (MSVC)")
    endif()
endif()

add_library(echoshade_simd INTERFACE)
if(ECHOSHADE_HAS_AVX2 AND NOT MSVC)
    target_compile_options(echoshade_simd INTERFACE -mavx2 -mfma)
elseif(ECHOSHADE_HAS_SSE42 AND NOT MSVC)
    target_compile_options(echoshade_simd INTERFACE -msse4.2)
elseif(ECHOSHADE_HAS_AVX2 AND MSVC)
    target_compile_options(echoshade_simd INTERFACE /arch:AVX2)
endif()
