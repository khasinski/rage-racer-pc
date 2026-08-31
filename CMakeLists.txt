cmake_minimum_required(VERSION 3.20)

project(rage_racer_port VERSION 0.6.1 LANGUAGES C)

set(RAGE_RACER_RELEASE_VERSION "0.6.1-alpha")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# The retail data segment, split so each subsystem's state sits with the code
# that reads it. Nothing links one of these without the rest: they were one
# object file in retail and they still describe one program's state, so every
# target that needs retail state takes the whole list.
set(RAGE_HOST_STATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_asset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_audio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_car.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_cd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_menu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_pad.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_race.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_render.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_save.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_track.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/port/host_state_unread.c
)

# The recovered game code relies on wrapping signed arithmetic and contains
# deliberate type punning. Keep optimized host builds from treating either as
# permission to rewrite game behavior.
if(MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # clang-cl accepts GNU driver flags only through its /clang: escape hatch.
    # Passing them directly produces -Wunknown-argument and breaks the Windows
    # release build once the port targets enable -Werror.
    set(RAGE_COMPAT_COMPILE_OPTIONS
        /clang:-fno-strict-aliasing
        /clang:-fwrapv)
    set(RAGE_STRICT_WARNING_OPTIONS
        /clang:-Wall
        /clang:-Wextra
        /clang:-Werror)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    set(RAGE_COMPAT_COMPILE_OPTIONS -fno-strict-aliasing -fwrapv)
    set(RAGE_STRICT_WARNING_OPTIONS -Wall -Wextra -Werror)
endif()

# Link the Visual C++ runtime statically. Windows does not ship it, and an
# archive that expects the player to install a redistributable first fails
# before main() with STATUS_DLL_NOT_FOUND and no log to explain why.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # The port intentionally uses the ISO C file APIs shared by every host.
    # Microsoft's CRT deprecates those names unless the application explicitly
    # opts into the portable interface.
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

include(CTest)

option(RAGE_BUILD_PORT "Build the native Rage Racer port" ON)
option(RAGE_ENABLE_SANITIZERS "Build host targets with ASan and UBSan" OFF)
option(RAGE_ENABLE_LINT "Expose the host-code lint target" ON)
set(RAGE_RUNTIME_DISC "" CACHE FILEPATH
    "Retail CUE/BIN/CHD used by optional native runtime tests")

# clang-tidy needs the exact per-file compile commands.  The setting is a
# no-op for generators that do not support a compilation database.
if(RAGE_ENABLE_LINT)
    if(CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_custom_target(lint
                COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_lint.py
                    --build-dir ${CMAKE_BINARY_DIR}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Linting native port and renderer code")
        else()
            message(STATUS "Lint target disabled: Python 3 interpreter not found")
        endif()
    else()
        message(STATUS "Lint target disabled: generator has no compilation database")
    endif()
endif()

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
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rage-sim PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()

add_library(rage-rmesh STATIC
    src/render/asset_id.c
    src/render/car_paint.c
    src/render/mod_manifest.c
    src/render/render_material.c
    src/render/texture_mipmap.c
    src/render/rmesh.c
    src/render/rmesh_index.c
    src/render/rmesh_cache.c)
target_include_directories(rage-rmesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(NOT MSVC)
    target_link_libraries(rage-rmesh PUBLIC m)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rage-rmesh PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()

add_library(rage-render-world STATIC
    src/render/render_world.c
    src/render/render_world_frame.c
    src/render/render_world_snapshot.c
    src/render/render_projection.c
    src/render/render_stage.c
    src/render/render_shadow.c
    src/render/render_mesh_build.c)
target_include_directories(rage-render-world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(NOT MSVC)
    target_link_libraries(rage-render-world PUBLIC m)
endif()
target_link_libraries(rage-render-world PUBLIC rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rage-render-world PRIVATE
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()

if(RAGE_BUILD_PORT)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CHDR_WANT_TESTS OFF CACHE BOOL "" FORCE)
    set(CHDR_WANT_SUBCODE OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/libchdr external-libchdr)
    add_subdirectory(external/psyz/psyz external-psyz)

    file(GLOB_RECURSE RAGE_GAME_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main/PAL/main/*.c"
    )
    list(REMOVE_ITEM RAGE_GAME_SOURCES ${RAGE_SIM_SOURCES})

    add_library(rage-game-full OBJECT ${RAGE_GAME_SOURCES} src/port/native_game_state.c)
    add_library(rage-host-state OBJECT
        ${RAGE_HOST_STATE_SOURCES}
        src/port/native_initialized_state.c
    )
    # Retail state is defined with the types its headers declare, so this
    # needs the s32/u16 family the rest of the tree is written in.
    target_include_directories(rage-host-state PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    # The retail game and its host-state bridge intentionally preserve signed
    # wrapping and type-punned PS1 storage. New renderer and mod code must not
    # inherit those relaxed assumptions.
    target_compile_options(rage-game-full PRIVATE ${RAGE_COMPAT_COMPILE_OPTIONS})
    target_compile_options(rage-host-state PRIVATE ${RAGE_COMPAT_COMPILE_OPTIONS})
    set(RAGE_PORT_COMMON_SOURCES
        src/port/disc_picker.c
        src/port/host_clock.c
        src/port/disc_stream_publish.c
        src/port/input_config.c
        src/port/input_device_select.c
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
        src/port/chase_camera.c
        src/port/content_options.c
        src/port/chd_disc.c
        src/port/disc_stream_table.c
        src/port/hud_config.c
        src/port/mod_assets.c
        src/port/texture_patch.c
        src/port/port_config.c
        src/port/scenario_control.c
        src/port/classic/native_geometry_diagnostics.c
        src/port/native_asset_importer.c
        src/port/modern/modern_renderer.c
        src/port/modern/modern_assets.c
        src/port/modern/modern_native_gpu.c
        src/port/modern/modern_shader_sources.c
        src/port/modern/modern_texture_dump.c
        src/port/modern/modern_renderer_diagnostics.c
        src/port/modern/scene_capture.c
    )
    set(RAGE_PORT_LEGACY_SOURCES
        src/port/platform_stubs.c
        src/port/legacy_platform.c
        src/port/classic/native_geometry.c
        src/port/native_geometry_interpolation.c
        src/port/native_asset_loader.c
        src/port/fmv_host.c
    )
    add_library(rage-port-legacy OBJECT ${RAGE_PORT_LEGACY_SOURCES})
    target_include_directories(rage-port-legacy PRIVATE src/port/include include)
    target_link_libraries(rage-port-legacy PRIVATE psyz)
    target_compile_options(rage-port-legacy PRIVATE ${RAGE_COMPAT_COMPILE_OPTIONS})
    add_executable(rage-racer
        MACOSX_BUNDLE
        src/port/main.c
        src/port/runtime_control.c
        ${RAGE_PORT_COMMON_SOURCES}
        $<TARGET_OBJECTS:rage-port-legacy>
        $<TARGET_OBJECTS:rage-game-full>
        $<TARGET_OBJECTS:rage-host-state>
    )
    add_executable(rage-racer-smoke
        src/port/main_smoke.c
        src/port/smoke_control.c
        ${RAGE_PORT_COMMON_SOURCES}
        $<TARGET_OBJECTS:rage-port-legacy>
        $<TARGET_OBJECTS:rage-game-full>
        $<TARGET_OBJECTS:rage-host-state>
    )
    target_compile_definitions(rage-racer-smoke PRIVATE RAGE_SMOKE_TARGET=1)
    add_executable(rage-extract tools/rage_extract.c)
    add_executable(rage-pack tools/rage_pack.c src/port/texture_patch.c)
    target_include_directories(rage-pack PRIVATE src/port)
    add_executable(rage-save-generator tools/rage_save_generator.c)
    add_executable(rage-frame-replay
        tools/rage_frame_replay.c
        src/port/platform_paths.c
        src/port/runtime_config.c
        src/port/mod_assets.c
        src/port/texture_patch.c
        src/port/track_asset_identity.c
        src/port/native_asset_importer_stubs.c
        src/port/modern/modern_assets.c
        src/port/modern/modern_native_gpu.c
        src/port/modern/modern_texture_dump.c)
    add_executable(rage-render-stage
        tools/rage_render_stage.c
        src/port/platform_paths.c
        src/port/runtime_config.c
        src/port/mod_assets.c
        src/port/texture_patch.c
        src/port/track_asset_identity.c
        src/port/native_asset_importer_stubs.c
        src/port/modern/modern_assets.c
        src/port/modern/modern_native_gpu.c
        src/port/modern/modern_texture_dump.c)
    target_include_directories(rage-render-stage PRIVATE
        src src/port src/port/include)
    target_link_libraries(rage-render-stage PRIVATE
        rage-render-world SDL3::SDL3)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(rage-render-stage PRIVATE
            ${RAGE_STRICT_WARNING_OPTIONS})
    endif()
    target_include_directories(rage-frame-replay PRIVATE
        src src/port src/port/include)
    target_link_libraries(rage-frame-replay PRIVATE
        rage-render-world SDL3::SDL3)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(rage-frame-replay PRIVATE
            ${RAGE_STRICT_WARNING_OPTIONS})
    endif()
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
            rage-sim psyz rage-render-world chdr-static)
        if(WIN32)
            target_link_libraries(${RAGE_PORT_TARGET} PRIVATE comdlg32 legacy_stdio_definitions)
        endif()
    endforeach()
    if(RAGE_ENABLE_SANITIZERS AND
       CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
        foreach(RAGE_SANITIZER_TARGET rage-game-full rage-host-state rage-port-legacy
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
        foreach(RAGE_MSVC_TARGET rage-game-full rage-host-state rage-port-legacy)
            target_compile_options(${RAGE_MSVC_TARGET} PRIVATE
                "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/port/include/rage/msvc_compat.h"
            )
        endforeach()
    endif()
    if(APPLE)
        set(RAGE_MACOS_ICON
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/RageRacer.icns")
        set_source_files_properties("${RAGE_MACOS_ICON}" PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
        target_sources(rage-racer PRIVATE "${RAGE_MACOS_ICON}")
        set_target_properties(rage-racer PROPERTIES
            OUTPUT_NAME "Rage Racer"
            MACOSX_BUNDLE_BUNDLE_NAME "Rage Racer"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.khasinski.rage-racer"
            MACOSX_BUNDLE_BUNDLE_VERSION "${RAGE_RACER_RELEASE_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${RAGE_RACER_RELEASE_VERSION}"
            MACOSX_BUNDLE_ICON_FILE "RageRacer.icns"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
        )

        # Older build trees contain a standalone `rage-racer` left behind
        # from before this target became a macOS application bundle.  That
        # executable is not updated by the bundle target and can silently run
        # stale game, renderer and audio code.  Keep the familiar developer
        # path, but make it resolve to the executable CMake actually builds.
        set(RAGE_MACOS_BUILD_LAUNCHER
            "${CMAKE_CURRENT_BINARY_DIR}/rage-racer")
        file(REMOVE "${RAGE_MACOS_BUILD_LAUNCHER}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E create_symlink
                "Rage Racer.app/Contents/MacOS/Rage Racer"
                "${RAGE_MACOS_BUILD_LAUNCHER}"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    if(WIN32)
        target_sources(rage-racer PRIVATE
            packaging/windows/rage-racer.rc)
    endif()
    if(BUILD_TESTING)
        add_executable(rage-gte-replay tools/rage_gte_replay.c)
        target_link_libraries(rage-gte-replay PRIVATE psyz)
        add_executable(rage-gp0-replay tools/rage_gp0_replay.c)
        target_link_libraries(rage-gp0-replay PRIVATE psyz)
        add_executable(rage-hud-preview
            tools/rage_hud_preview.c
            src/main/PAL/main/render/build_race_hud_prims.c
            src/main/PAL/main/render/race_hud.c
            src/main/PAL/main/render/text.c
            src/main/PAL/main/render/draw_packet_queue.c
            src/main/PAL/main/race/race_hud_text.c
            src/port/native_game_state.c
            src/port/hud_config.c
            $<TARGET_OBJECTS:rage-host-state>)
        target_include_directories(rage-hud-preview PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/port/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src/port
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(rage-hud-preview PRIVATE psyz)
        target_compile_options(rage-hud-preview PRIVATE
            ${RAGE_COMPAT_COMPILE_OPTIONS})
        if(NOT WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(rage-hud-preview PRIVATE -fcommon)
        endif()
        add_test(NAME save_generator
            COMMAND rage-save-generator --self-test)
        set_tests_properties(save_generator PROPERTIES LABELS functional)
        if(RAGE_RUNTIME_DISC)
            add_test(NAME attract_runtime
                COMMAND "${CMAKE_COMMAND}" -E env
                    "SDL_AUDIODRIVER=dummy"
                    "RAGE_PORT_DISC_CUE=${RAGE_RUNTIME_DISC}"
                    $<TARGET_FILE:rage-racer-smoke>
                    --set video.renderer=modern
                    --set race.enabled=false
                    --set boot.direct=false
                    --set run.frames=3500
                    --set stop.scene=30
                    --set stop.timer=400
                    --set diagnostics.log=stderr)
            set_tests_properties(attract_runtime PROPERTIES
                LABELS "retail;runtime"
                TIMEOUT 180)
        endif()
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
