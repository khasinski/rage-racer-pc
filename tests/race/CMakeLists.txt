add_executable(prologue_logic_tests
    prologue_logic_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/prologue_logic.c)
target_include_directories(prologue_logic_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(prologue_logic_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(prologue_logic_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(prologue_logic_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME prologue_logic COMMAND prologue_logic_tests)
set_tests_properties(prologue_logic PROPERTIES LABELS unit)

add_executable(record_defaults_tests
    record_defaults_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/records.c)
target_include_directories(record_defaults_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(record_defaults_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(record_defaults_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(record_defaults_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(record_defaults_tests PRIVATE
        -Wall -Wextra -Wno-deprecated-non-prototype -Werror)
endif()
add_test(NAME record_defaults COMMAND record_defaults_tests)

set_tests_properties(record_defaults PROPERTIES LABELS functional)

add_executable(save_checksum_tests
    save_checksum_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_checksum.c)
target_include_directories(save_checksum_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(save_checksum_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(save_checksum_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(save_checksum_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(save_checksum_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME save_checksum COMMAND save_checksum_tests)
set_tests_properties(save_checksum PROPERTIES LABELS unit)

if(TARGET rage-host-state)
add_executable(save_state_roundtrip_tests
    save_state_roundtrip_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/load_save_state.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/store_save_state.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_checksum.c)
target_include_directories(save_state_roundtrip_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(save_state_roundtrip_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(save_state_roundtrip_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(save_state_roundtrip_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(save_state_roundtrip_tests PRIVATE rage-host-state psyz)
add_test(NAME save_state_roundtrip COMMAND save_state_roundtrip_tests)
set_tests_properties(save_state_roundtrip PROPERTIES LABELS functional)
endif()

add_executable(course_index_tests course_index_tests.c)
target_include_directories(course_index_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/support/include
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(course_index_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME course_index COMMAND course_index_tests)

set_tests_properties(course_index PROPERTIES LABELS functional)

add_executable(waypoint_update_tests
    waypoint_update_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/waypoint_update.c)
target_include_directories(waypoint_update_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(waypoint_update_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(waypoint_update_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(waypoint_update_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(waypoint_update_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME waypoint_update COMMAND waypoint_update_tests)
set_tests_properties(waypoint_update PROPERTIES LABELS functional)

if(RAGE_RUBY_EXECUTABLE)
add_test(
    NAME race_progress_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_progress_layout.rb
        ${CMAKE_SOURCE_DIR}
)

set_tests_properties(race_progress_layout PROPERTIES LABELS functional)
endif()

if(TARGET rage-host-state)
add_executable(lap_and_finish_tests
    lap_and_finish_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/lap_and_finish.c)
target_include_directories(lap_and_finish_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(lap_and_finish_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(lap_and_finish_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(lap_and_finish_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(lap_and_finish_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(lap_and_finish_tests PRIVATE psyz)
add_test(NAME lap_and_finish COMMAND lap_and_finish_tests)
set_tests_properties(lap_and_finish PROPERTIES LABELS functional)
endif()

add_executable(bgm_select_control_tests
    bgm_select_control_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_select_control.c)
target_include_directories(bgm_select_control_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(bgm_select_control_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(bgm_select_control_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(bgm_select_control_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(bgm_select_control_tests PRIVATE psyz)
add_test(NAME bgm_select_control COMMAND bgm_select_control_tests)
set_tests_properties(bgm_select_control PROPERTIES LABELS functional)

add_executable(bgm_select_transition_tests
    bgm_select_transition_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_select_transition.c)
target_include_directories(bgm_select_transition_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(bgm_select_transition_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(bgm_select_transition_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(bgm_select_transition_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(bgm_select_transition_tests PRIVATE psyz)
add_test(NAME bgm_select_transition COMMAND bgm_select_transition_tests)
set_tests_properties(bgm_select_transition PROPERTIES LABELS functional)

add_executable(series_cleared_wash_tests
    series_cleared_wash_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/series_cleared_wash.c)
target_include_directories(series_cleared_wash_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(series_cleared_wash_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(series_cleared_wash_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(series_cleared_wash_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(series_cleared_wash_tests PRIVATE psyz)
add_test(NAME series_cleared_wash COMMAND series_cleared_wash_tests)
set_tests_properties(series_cleared_wash PROPERTIES LABELS unit)

add_executable(race_time_panel_tests
    race_time_panel_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/race_time_panel.c)
target_include_directories(race_time_panel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(race_time_panel_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(race_time_panel_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(race_time_panel_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(race_time_panel_tests PRIVATE psyz)
add_test(NAME race_time_panel COMMAND race_time_panel_tests)
set_tests_properties(race_time_panel PROPERTIES LABELS functional)

add_executable(bgm_select_bar_tests
    bgm_select_bar_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_select_bar.c)
target_include_directories(bgm_select_bar_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(bgm_select_bar_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(bgm_select_bar_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(bgm_select_bar_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(bgm_select_bar_tests PRIVATE psyz)
add_test(NAME bgm_select_bar COMMAND bgm_select_bar_tests)
set_tests_properties(bgm_select_bar PROPERTIES LABELS unit)

if(TARGET rage-racer-smoke)
    add_test(
        NAME scenario_control
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scenario_control.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(scenario_control PROPERTIES LABELS e2e)
    set_tests_properties(scenario_control PROPERTIES TIMEOUT 150)
    add_test(
        NAME scenario_after_finish
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scenario_after_finish.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(scenario_after_finish PROPERTIES LABELS e2e)
    set_tests_properties(scenario_after_finish PROPERTIES TIMEOUT 210)
    add_test(
        NAME course_matrix
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_course_matrix.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(course_matrix PROPERTIES LABELS e2e)
    set_tests_properties(course_matrix PROPERTIES TIMEOUT 1080)
    add_test(
        NAME track_start
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_track_start.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(track_start PROPERTIES LABELS e2e)
    set_tests_properties(track_start PROPERTIES TIMEOUT 165)
    add_test(
        NAME initial_state
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_initial_state.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(initial_state PROPERTIES LABELS e2e)
    set_tests_properties(initial_state PROPERTIES TIMEOUT 60)

    add_test(
        NAME race_start
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_start.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(race_start PROPERTIES LABELS e2e)
    set_tests_properties(race_start PROPERTIES TIMEOUT 150)
    add_test(
        NAME race_restart
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_restart.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(race_restart PROPERTIES LABELS e2e)
    set_tests_properties(race_restart PROPERTIES TIMEOUT 195)

    add_test(
        NAME grand_prix_results
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_results.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(grand_prix_results PROPERTIES LABELS e2e)
    set_tests_properties(grand_prix_results PROPERTIES TIMEOUT 225
        SKIP_RETURN_CODE 77)

    add_test(
        NAME grand_prix_exit
        COMMAND ruby
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_exit.rb
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(grand_prix_exit PROPERTIES LABELS e2e)
    set_tests_properties(grand_prix_exit PROPERTIES TIMEOUT 225)

    add_test(
        NAME time_attack_records
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_time_attack_records.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(time_attack_records PROPERTIES LABELS e2e)
    set_tests_properties(time_attack_records PROPERTIES TIMEOUT 225)

    add_test(
        NAME save_roundtrip
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_save_roundtrip.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
            $<TARGET_FILE:rage-save-generator>
    )
set_tests_properties(save_roundtrip PROPERTIES LABELS e2e)
    set_tests_properties(save_roundtrip PROPERTIES TIMEOUT 270)
endif()
