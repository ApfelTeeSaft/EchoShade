if(MSVC)
    add_compile_options(
        /W4
        /WX-
        /permissive-
        /Zc:__cplusplus
        "$<$<CONFIG:Release>:/O2>"
        "$<$<CONFIG:Release>:/GL>"
        "$<$<CONFIG:Debug>:/Od>"
        "$<$<CONFIG:Debug>:/Zi>"
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        "$<$<CONFIG:Release>:-O3>"
        "$<$<CONFIG:Release>:-DNDEBUG>"
        "$<$<CONFIG:Release>:-ffast-math>"
        "$<$<CONFIG:Debug>:-O0>"
        "$<$<CONFIG:Debug>:-g3>"
    )
endif()

if(ECHOSHADE_ENABLE_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "EchoShade: AddressSanitizer enabled")
endif()

if(ECHOSHADE_ENABLE_TSAN AND NOT MSVC)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
    message(STATUS "EchoShade: ThreadSanitizer enabled")
endif()

if(NOT MSVC)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_out)
    if(_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()
endif()
