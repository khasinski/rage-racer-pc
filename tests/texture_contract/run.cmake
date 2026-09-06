cmake_minimum_required(VERSION 3.20)
# Build only the pinned SDL dependency, production tools and C fixtures.
get_filename_component(source "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
if(NOT DEFINED RAGE_TEXTURE_CONTRACT_BUILD_DIR)
    set(RAGE_TEXTURE_CONTRACT_BUILD_DIR "${source}/build/texture-ci")
endif()
get_filename_component(build "${RAGE_TEXTURE_CONTRACT_BUILD_DIR}" ABSOLUTE)
if(NOT DEFINED RAGE_TEXTURE_SDL_SOURCE)
    set(RAGE_TEXTURE_SDL_SOURCE "${source}/external/psyz/external/SDL")
endif()
if(CMAKE_HOST_WIN32)
    set(generator -G "Visual Studio 17 2022" -A x64 -T ClangCL)
else()
    set(generator -G Ninja)
endif()
function(checked)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "Contract command failed (${result}): ${ARGV}")
    endif()
endfunction()
checked("${CMAKE_COMMAND}" -S "${RAGE_TEXTURE_SDL_SOURCE}" -B "${build}/sdl"
    ${generator} -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=OFF -DSDL_STATIC=ON
    -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_UNIX_CONSOLE_BUILD=ON
    -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_KMSDRM=OFF -DSDL_AUDIO=OFF)
checked("${CMAKE_COMMAND}" --build "${build}/sdl" --config Release --parallel 4)
checked("${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${build}/contracts"
    ${generator} -DCMAKE_BUILD_TYPE=Release -DRAGE_TEXTURE_ARCHIVE_TESTS=ON
    "-DSDL3_DIR=${build}/sdl")
checked("${CMAKE_COMMAND}" --build "${build}/contracts" --config Release --parallel 4)
get_filename_component(cmake_bin "${CMAKE_COMMAND}" DIRECTORY)
checked("${cmake_bin}/ctest" --test-dir "${build}/contracts" -C Release --output-on-failure)
checked("${CMAKE_COMMAND}" -S "${source}/tests/native_fixture_contract"
    -B "${build}/native-fixtures" ${generator} -DCMAKE_BUILD_TYPE=Release
    "-DSDL3_DIR=${build}/sdl")
checked("${CMAKE_COMMAND}" --build "${build}/native-fixtures" --config Release --parallel 4)
checked("${cmake_bin}/ctest" --test-dir "${build}/native-fixtures" -C Release --output-on-failure)
