cmake_minimum_required(VERSION 3.20)

project(rage_racer_port VERSION 0.1.0 LANGUAGES C)

set(RAGE_RACER_RELEASE_VERSION "0.1-alpha")

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
        src/port/platform_stubs.c
        src/port/legacy_platform.c
        src/port/native_geometry.c
        src/port/fmv_host.c
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
    target_include_directories(rage-game-full PRIVATE
        src/port/include
        include
    )
    target_link_libraries(rage-game-full PUBLIC psyz)
    # The recovered game sources retain PS1-era tentative globals.  Host
    # initialized state deliberately provides their concrete definitions; GCC
    # otherwise treats the tentative copies as duplicate ELF definitions.
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(rage-game-full PRIVATE -fcommon)
    endif()
    foreach(RAGE_PORT_TARGET rage-racer rage-racer-smoke)
        target_include_directories(${RAGE_PORT_TARGET} PRIVATE src/port/include)
        target_compile_options(${RAGE_PORT_TARGET} PRIVATE
            "-iquote${CMAKE_CURRENT_SOURCE_DIR}/src/port/include"
            "-iquote${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        target_link_libraries(${RAGE_PORT_TARGET} PRIVATE psyz)
        if(WIN32)
            target_link_libraries(${RAGE_PORT_TARGET} PRIVATE comdlg32)
        endif()
    endforeach()
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
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
