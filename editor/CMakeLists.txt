# The save editor builds on its own: it needs the game's save layout header,
# not the game.
cmake_minimum_required(VERSION 3.16)
project(rage-save-editor C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(RAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH
    "The port's source tree, for the save layout header")
set(RAGE_CIMGUI "${RAGE_ROOT}/external/psyz/external/cimgui")
set(RAGE_SDL "${RAGE_ROOT}/external/psyz/external/SDL")

foreach(needed
    "${RAGE_ROOT}/include/game/save_format.h"
    "${RAGE_CIMGUI}/cimgui.h"
    "${RAGE_SDL}/CMakeLists.txt")
  if(NOT EXISTS "${needed}")
    message(FATAL_ERROR
        "missing ${needed}. Run this from a checkout with its submodules, or "
        "point RAGE_ROOT at one.")
  endif()
endforeach()

find_package(OpenGL REQUIRED)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory("${RAGE_SDL}" sdl EXCLUDE_FROM_ALL)
# Static: the ImGui backends this program compiles call ImGui's own C++
# entry points, and a shared cimgui exports only its C wrapper, which leaves
# those undefined at link time on Windows.
set(IMGUI_STATIC "yes" CACHE STRING "" FORCE)
add_subdirectory("${RAGE_CIMGUI}" cimgui EXCLUDE_FROM_ALL)
target_compile_definitions(cimgui PUBLIC CIMGUI_USE_OPENGL3 CIMGUI_USE_SDL3)

# The layout header reaches for the port's own basic types.
set(RAGE_FORMAT_INCLUDES
    "${RAGE_ROOT}/src/port/include"
    "${RAGE_ROOT}/include"
    "${RAGE_ROOT}/external/psyz/psyz/include")

add_library(rage-save-format STATIC src/rage_save.c)
target_include_directories(rage-save-format PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src" ${RAGE_FORMAT_INCLUDES})
target_compile_definitions(rage-save-format PUBLIC __psyz)
if(NOT MSVC)
  target_compile_options(rage-save-format PRIVATE -Wall -Wextra)
endif()

add_executable(rage-save-editor
    src/main.c
    src/editor_ui.c
    "${RAGE_CIMGUI}/imgui/backends/imgui_impl_opengl3.cpp"
    "${RAGE_CIMGUI}/imgui/backends/imgui_impl_sdl3.cpp")
# cimgui's header only declares its structs and enums to C when asked.
target_compile_definitions(rage-save-editor PRIVATE
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS CIMGUI_USE_OPENGL3 CIMGUI_USE_SDL3)
target_include_directories(rage-save-editor PRIVATE
    "${RAGE_CIMGUI}"
    "${RAGE_CIMGUI}/imgui"
    "${RAGE_CIMGUI}/imgui/backends")
target_link_libraries(rage-save-editor PRIVATE
    rage-save-format cimgui SDL3::SDL3-static OpenGL::GL)
if(NOT MSVC)
  target_compile_options(rage-save-editor PRIVATE -Wall -Wextra)
endif()
if(APPLE)
  set_target_properties(rage-save-editor PROPERTIES MACOSX_BUNDLE FALSE)
endif()

enable_testing()
add_executable(rage-save-format-tests
    tests/rage_save_tests.c
    "${RAGE_ROOT}/src/main/PAL/main/save/save_checksum.c")
target_link_libraries(rage-save-format-tests PRIVATE rage-save-format)
target_include_directories(rage-save-format-tests PRIVATE
    "${RAGE_ROOT}/external/psyz/external/SDL/include")
if(NOT MSVC)
  target_compile_options(rage-save-format-tests PRIVATE
      "-include${RAGE_ROOT}/src/port/include/rage/compat.h"
      -Wall -Wextra)
else()
  target_compile_options(rage-save-format-tests PRIVATE
      "/FI${RAGE_ROOT}/src/port/include/rage/compat.h")
endif()
add_test(NAME save_format COMMAND rage-save-format-tests)
