add_executable(game_logic_tests
    game_logic_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/input_config.c
    ${CMAKE_SOURCE_DIR}/src/port/port_config.c
    ${CMAKE_SOURCE_DIR}/src/port/runtime_config.c
    ${CMAKE_SOURCE_DIR}/src/port/game_diagnostics.c
    ${CMAKE_SOURCE_DIR}/src/port/platform_paths.c
)
target_link_libraries(game_logic_tests PRIVATE rage-sim)

target_include_directories(game_logic_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/support/include
    ${CMAKE_SOURCE_DIR}/src/port
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(game_logic_tests PRIVATE -Wall -Wextra -Werror)
endif()
if(RAGE_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(game_logic_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(game_logic_tests PRIVATE -fsanitize=address,undefined)
endif()

add_test(NAME game_logic COMMAND game_logic_tests)
set_tests_properties(game_logic PROPERTIES LABELS functional)

add_test(NAME architecture_boundaries COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_architecture_boundaries.py
    ${CMAKE_SOURCE_DIR})
set_tests_properties(architecture_boundaries PROPERTIES LABELS functional)

add_executable(content_options_tests
    content_options_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/content_options.c)
target_include_directories(content_options_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(content_options_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME content_options COMMAND content_options_tests)

set_tests_properties(content_options PROPERTIES LABELS functional)
# The bytes themselves, which the manifest above cannot see.
add_executable(host_state_content_tests
    host_state_content_tests.c
    ${RAGE_HOST_STATE_SOURCES})
target_include_directories(host_state_content_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include)
target_compile_definitions(host_state_content_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(host_state_content_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include")
endif()
add_test(NAME host_state_content COMMAND host_state_content_tests)
set_tests_properties(host_state_content PROPERTIES LABELS functional)

add_test(NAME host_state_abi COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_host_state_abi.py ${CMAKE_SOURCE_DIR})

set_tests_properties(host_state_abi PROPERTIES LABELS functional)
add_test(NAME shipped_config COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_shipped_config.py ${CMAKE_SOURCE_DIR})

set_tests_properties(shipped_config PROPERTIES LABELS functional)
if(APPLE AND TARGET rage-racer)
    add_test(NAME macos_build_launcher COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_macos_build_launcher.py
        ${CMAKE_BINARY_DIR}/rage-racer
        $<TARGET_FILE:rage-racer>)
    set_tests_properties(macos_build_launcher PROPERTIES LABELS functional)
endif()

# These tests need neither a staged disc image nor a windowing session and are
# therefore safe in every release-packaging job. Runtime tests below exercise
# the real PAL data in local/pre-release validation instead.
add_executable(timing_restore_tests timing_restore_tests.c ${CMAKE_SOURCE_DIR}/src/port/timing_restore.c)
target_include_directories(timing_restore_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port ${CMAKE_SOURCE_DIR}/src/port/include)
add_test(NAME timing_restore COMMAND timing_restore_tests)
set_tests_properties(timing_restore PROPERTIES LABELS functional)

if(TARGET rage-racer-smoke)
    add_test(
        NAME runtime_config
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_runtime_config.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(runtime_config PROPERTIES LABELS e2e)
    set_tests_properties(runtime_config PROPERTIES TIMEOUT 180)
    add_test(
        NAME diagnostic_log
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_diagnostic_log.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(diagnostic_log PROPERTIES LABELS e2e)
    set_tests_properties(diagnostic_log PROPERTIES TIMEOUT 60)
    add_test(
        NAME window_size
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_window_size.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(window_size PROPERTIES LABELS e2e)
    set_tests_properties(window_size PROPERTIES TIMEOUT 60)
    find_program(RAGE_XDOTOOL xdotool)
    if(RAGE_XDOTOOL)
        add_test(
            NAME window_lifecycle
            COMMAND python3
                ${CMAKE_CURRENT_SOURCE_DIR}/verify_window_lifecycle.py
                $<TARGET_FILE:rage-racer-smoke>
                ${CMAKE_SOURCE_DIR}
        )
set_tests_properties(window_lifecycle PROPERTIES LABELS e2e)
        set_tests_properties(window_lifecycle PROPERTIES TIMEOUT 195)
    endif()
endif()
add_executable(asset_requests_tests
    asset_requests_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/asset/asset_requests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/asset/option_screen_assets.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/asset/round_screen_assets.c)
target_include_directories(asset_requests_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(asset_requests_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(asset_requests_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(asset_requests_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(asset_requests_tests PRIVATE psyz)
add_test(NAME asset_requests COMMAND asset_requests_tests)
set_tests_properties(asset_requests PROPERTIES LABELS unit)
