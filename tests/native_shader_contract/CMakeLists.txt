cmake_minimum_required(VERSION 3.20)
project(RageNativeShaderContract LANGUAGES C)
enable_testing()
option(RAGE_REQUIRE_GPU "Fail instead of skipping when no GPU backend is available" OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_package(SDL3 CONFIG REQUIRED)
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(native_fog_gpu_tests
    "${ROOT}/tests/render/native_fog_gpu_tests.c"
    "${ROOT}/src/render/render_projection.c")
add_executable(native_shadow_mask_tests
    "${ROOT}/tests/render/native_shadow_mask_tests.c")
foreach(target native_fog_gpu_tests native_shadow_mask_tests)
    target_include_directories(${target} PRIVATE "${ROOT}/src"
        "${ROOT}/src/port/modern/shaders")
    target_link_libraries(${target} PRIVATE SDL3::SDL3)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
        target_link_libraries(${target} PRIVATE m)
    endif()
endforeach()
add_test(NAME native_fog_gpu COMMAND native_fog_gpu_tests)
add_test(NAME native_shadow_uv_gpu COMMAND native_fog_gpu_tests --shadow)
add_test(NAME native_shadow_mask_gpu COMMAND native_shadow_mask_tests)
set_tests_properties(native_fog_gpu native_shadow_uv_gpu native_shadow_mask_gpu
    PROPERTIES LABELS gpu TIMEOUT 30)
if(NOT RAGE_REQUIRE_GPU)
    set_tests_properties(native_fog_gpu native_shadow_uv_gpu native_shadow_mask_gpu
        PROPERTIES SKIP_RETURN_CODE 77)
endif()
