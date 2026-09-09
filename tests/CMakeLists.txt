# Tests are grouped by what they are about, not by how they run. A unit test in
# C and the end-to-end script covering the same subsystem live in the same
# directory; ctest -L functional/e2e/tooling still answers how a test runs.
#
# Shared fixtures live in support/: common.h and the include/ tree the C tests
# compile against. The Asset Browser is isolated development tooling and keeps
# its own Python fixtures under tools/assetbrowser/.

# Test timeouts are wall-clock, and most of these run the game at its own pace
# rather than as fast as the machine can. race_cdda waits out an audio track and
# needs 66 seconds of real time; its limit was 45, so it failed whenever anything
# else was running. Every limit here is three times the time its test takes on an
# idle machine, which is enough that a busy one does not turn a passing test into
# a failing one. They still catch a hang, which is what they are for.
#
# The scripts carry their own subprocess timeouts as well, and those follow the
# same rule. They were left at one times the idle cost when these were raised,
# which made them the limit that actually bit: scenario_after_finish had 65
# seconds inside the script against 210 here, so a parallel run failed it while
# ctest was still waiting patiently.

# Visual-comparison helpers remain optional developer tools. Do not register
# them as failing tests on hosts that intentionally do not install the optional
# Ruby tooling used by visual-comparison helpers.
find_program(RAGE_RUBY_EXECUTABLE ruby)

if(TARGET rage-racer-smoke)
    foreach(region PAL NTSC-U NTSC-J)
        string(REPLACE "-" "_" key "${region}")
        string(TOLOWER "${key}" suffix)
        add_test(NAME modern_default_disc_source_${suffix}
            COMMAND ${CMAKE_COMMAND} -DGAME=$<TARGET_FILE:rage-racer-smoke>
                -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
                -DDISC_ENV=RAGE_PORT_${key}_CUE -DEXPECT_REGION=${region}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/render/verify_default_disc_source.cmake)
        set_tests_properties(modern_default_disc_source_${suffix} PROPERTIES
            LABELS "e2e;regional" TIMEOUT 120 SKIP_REGULAR_EXPRESSION "SKIP:")
    endforeach()
endif()

# Exercise the production entry point too: smoke bypasses normal disc startup
# and uses a different diagnostic-log destination.
if(TARGET rage-racer)
    foreach(region PAL NTSC-U NTSC-J)
        string(REPLACE "-" "_" key "${region}")
        string(TOLOWER "${key}" suffix)
        add_test(NAME production_disc_source_${suffix}
            COMMAND ${CMAKE_COMMAND} -DGAME=$<TARGET_FILE:rage-racer>
                -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
                -DDISC_ENV=RAGE_PORT_${key}_CUE -DEXPECT_REGION=${region}
                -DPRODUCTION_ENTRY=ON
                -P ${CMAKE_CURRENT_SOURCE_DIR}/render/verify_default_disc_source.cmake)
        set_tests_properties(production_disc_source_${suffix} PROPERTIES
            LABELS "e2e;regional" TIMEOUT 120 SKIP_REGULAR_EXPRESSION "SKIP:")
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_test(NAME production_saved_disc_${suffix}
                COMMAND ${CMAKE_COMMAND} -DGAME=$<TARGET_FILE:rage-racer>
                    -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
                    -DDISC_ENV=RAGE_PORT_${key}_CUE -DEXPECT_REGION=${region}
                    -DPRODUCTION_ENTRY=ON -DSAVED_DISC=ON
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/render/verify_default_disc_source.cmake)
            set_tests_properties(production_saved_disc_${suffix} PROPERTIES
                LABELS "e2e;regional" TIMEOUT 120 SKIP_REGULAR_EXPRESSION "SKIP:")
        endif()
    endforeach()
endif()

# A release build defines NDEBUG, which would turn the assertion-based C
# tests into no-ops. Keep assertions active in every test target while leaving
# the release game binaries untouched.
if(MSVC)
    add_compile_options(/UNDEBUG)
else()
    add_compile_options(-UNDEBUG)
endif()

# Instrument test translation units as well as linking the sanitizer runtime.
# Many tests compile production sources directly; runtime-only linking leaves
# those paths unchecked even in a sanitizer build.
if(RAGE_ENABLE_SANITIZERS AND
   CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Source contracts run as C binaries on release hosts without Python. Let
# CMake track additions to the same source trees the original checks scanned.
file(GLOB neutral_files CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/render/*.c" "${CMAKE_SOURCE_DIR}/src/render/*.h")
foreach(path IN LISTS RAGE_SIM_SOURCES)
    file(RELATIVE_PATH relative "${CMAKE_SOURCE_DIR}" "${path}")
    list(APPEND neutral_files "${relative}")
endforeach()
file(GLOB modern_files CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/port/modern/*.c" "${CMAKE_SOURCE_DIR}/src/port/modern/*.h")
file(GLOB_RECURSE retail_files CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/main/PAL/main/*.c")
file(GLOB host_state_files CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/port/host_state*.c")
set(contract_header "/* Generated source inventory; paths relative to the checkout. */\n")
foreach(group neutral_files modern_files retail_files host_state_files)
    string(APPEND contract_header "static const char *${group}[] = {\n")
    foreach(path IN LISTS ${group})
        string(APPEND contract_header "    \"${path}\",\n")
    endforeach()
    string(APPEND contract_header "    NULL\n};\n")
endforeach()
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/source_contract_files.h"
    CONTENT "${contract_header}" @ONLY)
add_executable(source_contract_tests support/source_contract_tests.c)
target_include_directories(source_contract_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(source_contract_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})

add_subdirectory(audio)
add_subdirectory(car)
add_subdirectory(disc)
add_subdirectory(input)
add_subdirectory(menu)
add_subdirectory(port)
add_subdirectory(race)
add_subdirectory(render)
include(render/regional_marker_tests.cmake)
add_subdirectory(tooling)
