cmake_minimum_required(VERSION 3.20)
project(RageModContractTests LANGUAGES C)
enable_testing()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(MSVC)
    # Match the production project's portable C runtime policy.
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
get_filename_component(RAGE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Standalone gate using the production sources and the SAME tests as the game
# build. No disc, SDL, generated car assets or GPU is needed on any platform.
add_library(json STATIC "${RAGE_SOURCE}/external/yyjson/yyjson.c")
target_include_directories(json PUBLIC "${RAGE_SOURCE}/external/yyjson")
add_library(mod_contract STATIC
    "${RAGE_SOURCE}/src/render/mod_package.c"
    "${RAGE_SOURCE}/src/render/mod_selection.c"
    "${RAGE_SOURCE}/src/render/mod_provider.c"
    "${RAGE_SOURCE}/src/render/mod_file_snapshot.c"
    "${RAGE_SOURCE}/src/render/mod_manifest.c"
    "${RAGE_SOURCE}/src/render/render_material.c"
    "${RAGE_SOURCE}/src/render/asset_path.c")
target_include_directories(mod_contract PUBLIC "${RAGE_SOURCE}/src")
target_link_libraries(mod_contract PRIVATE json)
if(MSVC)
    target_compile_options(mod_contract PRIVATE /W4 /WX)
else()
    target_compile_options(mod_contract PRIVATE -Wall -Wextra -Werror)
endif()
foreach(name mod_package mod_selection mod_provider mod_manifest mod_provider_stdin mod_file_snapshot)
    add_executable(${name}_tests "${RAGE_SOURCE}/tests/disc/${name}_tests.c")
    target_link_libraries(${name}_tests PRIVATE mod_contract)
    if(MSVC)
        target_compile_options(${name}_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(${name}_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME ${name} COMMAND ${name}_tests)
endforeach()
