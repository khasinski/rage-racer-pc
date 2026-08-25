cmake_minimum_required(VERSION 3.20)

project(rage_racer_port VERSION 0.4.0 LANGUAGES C)

set(RAGE_RACER_RELEASE_VERSION "0.4-alpha")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# The recovered game code relies on wrapping signed arithmetic and contains
# deliberate type punning. Keep optimized host builds from treating either as
# permission to rewrite game behavior.
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-strict-aliasing -fwrapv)
endif()

# Link the Visual C++ runtime statically. Windows does not ship it, and an
# archive that expects the player to install a redistributable first fails
# before main() with STATUS_DLL_NOT_FOUND and no log to explain why.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(CTest)

option(RAGE_BUILD_PORT "Build the native Rage Racer port" ON)
option(RAGE_ENABLE_SANITIZERS "Build host targets with ASan and UBSan" OFF)

# Deterministic gameplay helpers with no renderer, platform or PSY-Z link.
# Keeping this target usable when RAGE_BUILD_PORT=OFF gives unit tests a real
# simulation boundary instead of compiling selected game files ad hoc.
set(RAGE_SIM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/race/time_conversion.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/random/random15.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/track/blend_angle.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/track/interpolate_track_angle.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/track/lerp_color_channel.c
)
add_library(rage-sim STATIC ${RAGE_SIM_SOURCES})
target_include_directories(rage-sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(rage-sim PRIVATE RAGE_HOST_PORT=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rage-sim PRIVATE -Wall -Wextra -Werror)
endif()

add_library(rage-rmesh STATIC
    src/render/asset_id.c
    src/render/rmesh.c
    src/render/rmesh_index.c
    src/render/rmesh_cache.c)
target_include_directories(rage-rmesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(NOT MSVC)
    target_link_libraries(rage-rmesh PUBLIC m)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rage-rmesh PRIVATE -Wall -Wextra -Werror)
endif()

add_library(rage-render-world STATIC
    src/render/render_world.c
    src/render/render_world_frame.c
    src/render/render_projection.c
    src/render/render_mesh_build.c)
target_include_directories(rage-render-world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(NOT MSVC)
    target_link_libraries(rage-render-world PUBLIC m)
endif()
target_link_libraries(rage-render-world PUBLIC rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rage-render-world PRIVATE -Wall -Wextra -Werror)
endif()

if(RAGE_BUILD_PORT)
    add_subdirectory(external/psyz/psyz external-psyz)

    file(GLOB_RECURSE RAGE_GAME_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/*.c"
    )
    list(REMOVE_ITEM RAGE_GAME_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/boot/_start.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/render/matrix_apply.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/render/terrain_submission.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/asset/asset_loader.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/fmv/start_fmv_playback.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/fmv/fmv_decode.c"
        ${RAGE_SIM_SOURCES}
    )

    add_library(rage-game-full OBJECT ${RAGE_GAME_SOURCES} src/port/native_game_state.c)
    add_library(rage-host-state OBJECT
        src/port/host_state.c
        src/port/native_initialized_state.c
    )
    set(RAGE_PORT_COMMON_SOURCES
        src/port/input_config.c
        src/port/platform_paths.c
        src/port/diagnostic_log.c
        src/port/game_diagnostics.c
        src/port/rage_trace.c
        src/port/render_world_game.c
        src/port/track_asset_identity.c
        src/port/track_lighting.c
        src/port/runtime_config.c
        src/port/timing_control.c
        src/port/analog_pad.c
        src/port/axis_curve.c
        src/port/timing_restore.c
        src/port/mod_assets.c
        src/port/texture_patch.c
        src/port/port_config.c
        src/port/scenario_control.c
        src/port/platform_stubs.c
        src/port/legacy_platform.c
        src/port/classic/native_geometry.c
        src/port/classic/native_geometry_diagnostics.c
        src/port/native_geometry_interpolation.c
        src/port/native_asset_loader.c
        src/port/fmv_host.c
        src/port/modern/modern_renderer.c
        src/port/modern/modern_assets.c
        src/port/modern/modern_native_gpu.c
        src/port/modern/modern_shader_sources.c
        src/port/modern/modern_texture_dump.c
        src/port/modern/modern_renderer_diagnostics.c
        src/port/modern/scene_capture.c
    )
    add_executable(rage-racer
        MACOSX_BUNDLE
        src/port/main.c
        src/port/runtime_control.c
        ${RAGE_PORT_COMMON_SOURCES}
        $<TARGET_OBJECTS:rage-game-full>
        $<TARGET_OBJECTS:rage-host-state>
    )
    add_executable(rage-racer-smoke
        src/port/main_smoke.c
        src/port/smoke_control.c
        ${RAGE_PORT_COMMON_SOURCES}
        $<TARGET_OBJECTS:rage-game-full>
        $<TARGET_OBJECTS:rage-host-state>
    )
    target_compile_definitions(rage-racer-smoke PRIVATE RAGE_SMOKE_TARGET=1)
    add_executable(rage-extract tools/rage_extract.c)
    add_executable(rage-pack tools/rage_pack.c src/port/texture_patch.c)
    target_include_directories(rage-pack PRIVATE src/port)
    add_executable(rage-save-generator tools/rage_save_generator.c)
    target_compile_definitions(rage-save-generator PRIVATE __psyz=1)
    target_compile_options(rage-save-generator PRIVATE
        "-iquote${CMAKE_CURRENT_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_include_directories(rage-game-full PRIVATE
        src/port/include
        include
    )
    target_link_libraries(rage-game-full PUBLIC psyz)
    target_compile_definitions(rage-game-full PRIVATE RAGE_HOST_PORT=1)
    # The recovered game sources retain PS1-era tentative globals.  Host
    # initialized state deliberately provides their concrete definitions; GCC
    # otherwise treats the tentative copies as duplicate ELF definitions.
    if(NOT WIN32 AND
       (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_C_COMPILER_ID MATCHES "Clang"))
        target_compile_options(rage-game-full PRIVATE
            -fcommon -Wall -Wextra -Werror)
        if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang")
            target_compile_options(rage-game-full PRIVATE
                -Wno-error)
        endif()
    elseif(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(rage-game-full PRIVATE /clang:-fcommon)
    endif()
    foreach(RAGE_PORT_TARGET rage-racer rage-racer-smoke)
        target_include_directories(${RAGE_PORT_TARGET} PRIVATE
            src/port/include
            include
        )
        target_compile_definitions(${RAGE_PORT_TARGET} PRIVATE RAGE_HOST_PORT=1)
        if(NOT WIN32)
            target_compile_options(${RAGE_PORT_TARGET} PRIVATE
                "-iquote${CMAKE_CURRENT_SOURCE_DIR}/src/port/include"
                "-iquote${CMAKE_CURRENT_SOURCE_DIR}/include"
                -Wall -Wextra -Werror
            )
            # Apple Clang 16 diagnoses the recovered PS1-era declarations
            # without prototypes. Keep the diagnostics visible while the
            # legacy headers are incrementally typed, but do not fail release
            # builds solely on this compatibility warning.
            if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang")
                target_compile_options(${RAGE_PORT_TARGET} PRIVATE
                    -Wno-error=deprecated-non-prototype)
            endif()
        endif()
        target_link_libraries(${RAGE_PORT_TARGET} PRIVATE
            rage-sim psyz rage-render-world)
        if(WIN32)
            target_link_libraries(${RAGE_PORT_TARGET} PRIVATE comdlg32 legacy_stdio_definitions)
        endif()
    endforeach()
    if(RAGE_ENABLE_SANITIZERS AND
       CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
        foreach(RAGE_SANITIZER_TARGET rage-game-full rage-host-state
                                      rage-racer rage-racer-smoke)
            target_compile_options(${RAGE_SANITIZER_TARGET} PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer)
        endforeach()
        foreach(RAGE_SANITIZER_TARGET rage-racer rage-racer-smoke)
            target_link_options(${RAGE_SANITIZER_TARGET} PRIVATE
                -fsanitize=address,undefined)
        endforeach()
    endif()
    if(MSVC)
        target_compile_options(rage-game-full PRIVATE
            "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/port/include/rage/compat.h"
        )
    else()
        target_compile_options(rage-game-full PRIVATE
            "-include${CMAKE_CURRENT_SOURCE_DIR}/src/port/include/rage/compat.h"
        )
    endif()
    target_compile_definitions(psyz PRIVATE
        PSYZ_TITLE="Rage Racer"
        DEFAULT_FRONT_W=640
        DEFAULT_FRONT_H=480
    )
    if(MSVC)
        set_property(TARGET psyz PROPERTY C_STANDARD 11)
        set_property(TARGET psyz PROPERTY C_STANDARD_REQUIRED ON)
        target_compile_options(psyz PRIVATE "$<$<COMPILE_LANGUAGE:C>:/std:c11>")
        foreach(RAGE_MSVC_TARGET rage-game-full rage-host-state)
            target_compile_options(${RAGE_MSVC_TARGET} PRIVATE
                "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/port/include/rage/msvc_compat.h"
            )
        endforeach()
    endif()
    if(APPLE)
        set_target_properties(rage-racer PROPERTIES
            OUTPUT_NAME "Rage Racer"
            MACOSX_BUNDLE_BUNDLE_NAME "Rage Racer"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.khasinski.rage-racer"
            MACOSX_BUNDLE_BUNDLE_VERSION "${RAGE_RACER_RELEASE_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${RAGE_RACER_RELEASE_VERSION}"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
        )
    endif()
    if(BUILD_TESTING)
        add_executable(rage-gte-replay tools/rage_gte_replay.c)
        target_link_libraries(rage-gte-replay PRIVATE psyz)
        add_executable(rage-gp0-replay tools/rage_gp0_replay.c)
        target_link_libraries(rage-gp0-replay PRIVATE psyz)
        add_test(NAME save_generator
            COMMAND rage-save-generator --self-test)
        set_tests_properties(save_generator PROPERTIES LABELS portable)
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
