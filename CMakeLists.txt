cmake_minimum_required(VERSION 3.20)

project(rage_racer_port LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

include(CTest)

option(RAGE_BUILD_PORT "Build the native Rage Racer port" ON)

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
    )

    add_library(rage-game-full OBJECT ${RAGE_GAME_SOURCES} src/port/native_game_state.c)
    add_library(rage-host-state OBJECT
        src/port/host_state.c
        src/port/native_initialized_state.c
    )
    set(RAGE_PORT_COMMON_SOURCES
        src/port/input_config.c
        src/port/port_config.c
        src/port/platform_stubs.c
        src/port/legacy_platform.c
        src/port/native_geometry.c
        src/port/native_asset_loader.c
        src/port/fmv_host.c
        src/port/modern/modern_renderer.c
        src/port/modern/scene_capture.c
    )
    add_executable(rage-racer
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
    foreach(RAGE_PORT_TARGET rage-racer rage-racer-smoke)
        target_include_directories(${RAGE_PORT_TARGET} PRIVATE src/port/include)
        target_compile_options(${RAGE_PORT_TARGET} PRIVATE
            "-iquote${CMAKE_CURRENT_SOURCE_DIR}/src/port/include"
            "-iquote${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        target_link_libraries(${RAGE_PORT_TARGET} PRIVATE psyz)
    endforeach()
    target_compile_options(rage-game-full PRIVATE
        "-include${CMAKE_CURRENT_SOURCE_DIR}/src/port/include/rage/compat.h"
    )
    target_compile_definitions(psyz PRIVATE
        PSYZ_TITLE="Rage Racer"
        DEFAULT_FRONT_W=640
        DEFAULT_FRONT_H=480
    )
    if(BUILD_TESTING)
        add_executable(rage-gte-replay tools/rage_gte_replay.c)
        target_link_libraries(rage-gte-replay PRIVATE psyz)
        add_executable(rage-gp0-replay tools/rage_gp0_replay.c)
        target_link_libraries(rage-gp0-replay PRIVATE psyz)
        add_test(NAME save_generator
            COMMAND rage-save-generator --self-test)
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
