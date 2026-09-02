add_executable(class_progress_rules_tests
    class_progress_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/class_progress_rules.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/prize_money.c)
target_include_directories(class_progress_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(class_progress_rules_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(class_progress_rules_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(class_progress_rules_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME class_progress_rules COMMAND class_progress_rules_tests)
set_tests_properties(class_progress_rules PROPERTIES LABELS unit)

add_executable(class_win_state_tests
    class_win_state_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/class_win_state.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/class_progress_rules.c)
target_include_directories(class_win_state_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(class_win_state_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(class_win_state_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(class_win_state_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME class_win_state COMMAND class_win_state_tests)
set_tests_properties(class_win_state PROPERTIES LABELS unit)

add_executable(class_clear_fanfare_tests
    class_clear_fanfare_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/class_clear_fanfare.c)
target_include_directories(class_clear_fanfare_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(class_clear_fanfare_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(class_clear_fanfare_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(class_clear_fanfare_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME class_clear_fanfare COMMAND class_clear_fanfare_tests)
set_tests_properties(class_clear_fanfare PROPERTIES LABELS unit)

add_executable(advance_grand_prix_class_tests
    advance_grand_prix_class_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/advance_grand_prix_class.c)
target_include_directories(advance_grand_prix_class_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(advance_grand_prix_class_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(advance_grand_prix_class_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(advance_grand_prix_class_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME advance_grand_prix_class COMMAND advance_grand_prix_class_tests)
set_tests_properties(advance_grand_prix_class PROPERTIES LABELS unit)

add_executable(enter_prize_screen_tests
    enter_prize_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/enter_prize_screen.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/class_progress_rules.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/prize_money.c)
target_include_directories(enter_prize_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(enter_prize_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(enter_prize_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(enter_prize_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME enter_prize_screen COMMAND enter_prize_screen_tests)
set_tests_properties(enter_prize_screen PROPERTIES LABELS unit)

add_executable(commit_class_progress_tests
    commit_class_progress_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/commit_class_progress.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/class_progress_rules.c)
target_include_directories(commit_class_progress_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(commit_class_progress_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(commit_class_progress_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(commit_class_progress_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME commit_class_progress COMMAND commit_class_progress_tests)
set_tests_properties(commit_class_progress PROPERTIES LABELS unit)

add_executable(update_race_position_tests
    update_race_position_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/update_race_position.c)
target_include_directories(update_race_position_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(update_race_position_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(update_race_position_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(update_race_position_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME update_race_position COMMAND update_race_position_tests)
set_tests_properties(update_race_position PROPERTIES LABELS unit)

add_executable(race_time_text_tests
    race_time_text_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/race_time_text.c)
target_include_directories(race_time_text_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(race_time_text_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(race_time_text_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(race_time_text_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME race_time_text COMMAND race_time_text_tests)
set_tests_properties(race_time_text PROPERTIES LABELS unit)

add_executable(replay_playback_tests
    replay_playback_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/replay_playback.c)
target_include_directories(replay_playback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(replay_playback_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(replay_playback_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(replay_playback_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME replay_playback COMMAND replay_playback_tests)
set_tests_properties(replay_playback PROPERTIES LABELS unit)

add_executable(replay_recording_tests
    replay_recording_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/replay_recording.c)
target_include_directories(replay_recording_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(replay_recording_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(replay_recording_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(replay_recording_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME replay_recording COMMAND replay_recording_tests)
set_tests_properties(replay_recording PROPERTIES LABELS unit)

add_executable(replay_car_state_tests
    replay_car_state_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/replay_car_state.c)
target_include_directories(replay_car_state_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(replay_car_state_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(replay_car_state_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(replay_car_state_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME replay_car_state COMMAND replay_car_state_tests)
set_tests_properties(replay_car_state PROPERTIES LABELS unit)

add_executable(replay_start_tests
    replay_start_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/replay_start.c)
target_include_directories(replay_start_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(replay_start_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(replay_start_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(replay_start_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME replay_start COMMAND replay_start_tests)
set_tests_properties(replay_start PROPERTIES LABELS unit)

add_executable(replay_scene_rules_tests
    replay_scene_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/replay_scene_rules.c)
target_include_directories(replay_scene_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(replay_scene_rules_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(replay_scene_rules_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(replay_scene_rules_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME replay_scene_rules COMMAND replay_scene_rules_tests)
set_tests_properties(replay_scene_rules PROPERTIES LABELS unit)

add_executable(attract_demo_rules_tests
    attract_demo_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/attract_demo_rules.c)
target_include_directories(attract_demo_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(attract_demo_rules_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(attract_demo_rules_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(attract_demo_rules_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME attract_demo_rules COMMAND attract_demo_rules_tests)
set_tests_properties(attract_demo_rules PROPERTIES LABELS unit)

add_executable(bgm_track_tests
    bgm_track_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_track.c)
target_include_directories(bgm_track_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(bgm_track_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(bgm_track_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(bgm_track_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME bgm_track COMMAND bgm_track_tests)
set_tests_properties(bgm_track PROPERTIES LABELS unit)

add_executable(bgm_select_scene_tests
    bgm_select_scene_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_select.c)
target_include_directories(bgm_select_scene_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(bgm_select_scene_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(bgm_select_scene_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(bgm_select_scene_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME bgm_select_scene COMMAND bgm_select_scene_tests)
set_tests_properties(bgm_select_scene PROPERTIES LABELS unit)

add_executable(replay_fade_tests
    replay_fade_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/replay_fade.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/replay_scene_rules.c)
target_include_directories(replay_fade_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(replay_fade_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(replay_fade_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(replay_fade_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME replay_fade COMMAND replay_fade_tests)
set_tests_properties(replay_fade PROPERTIES LABELS unit)

add_executable(exit_race_scene_tests
    exit_race_scene_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/exit_race_scene.c)
target_include_directories(exit_race_scene_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(exit_race_scene_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(exit_race_scene_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(exit_race_scene_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME exit_race_scene COMMAND exit_race_scene_tests)
set_tests_properties(exit_race_scene PROPERTIES LABELS unit)

add_executable(enter_attract_scene_tests
    enter_attract_scene_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/enter_attract_scene.c)
target_include_directories(enter_attract_scene_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(enter_attract_scene_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(enter_attract_scene_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(enter_attract_scene_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME enter_attract_scene COMMAND enter_attract_scene_tests)
set_tests_properties(enter_attract_scene PROPERTIES LABELS unit)

add_executable(split_time_state_tests
    split_time_state_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/split_time_state.c)
target_include_directories(split_time_state_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(split_time_state_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(split_time_state_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(split_time_state_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME split_time_state COMMAND split_time_state_tests)
set_tests_properties(split_time_state PROPERTIES LABELS unit)

add_executable(start_countdown_timing_tests
    start_countdown_timing_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/start_countdown_timing.c)
target_include_directories(start_countdown_timing_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(start_countdown_timing_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME start_countdown_timing COMMAND start_countdown_timing_tests)
set_tests_properties(start_countdown_timing PROPERTIES LABELS unit)

add_executable(countdown_tiles_tests
    countdown_tiles_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/countdown_tiles.c)
target_include_directories(countdown_tiles_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(countdown_tiles_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(countdown_tiles_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(countdown_tiles_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME countdown_tiles COMMAND countdown_tiles_tests)
set_tests_properties(countdown_tiles PROPERTIES LABELS unit)

add_executable(race_option_animation_tests
    race_option_animation_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/race_option_animation.c)
target_include_directories(race_option_animation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(race_option_animation_tests PRIVATE __psyz=1)
target_link_libraries(race_option_animation_tests PRIVATE psyz)
if(MSVC)
    target_compile_options(race_option_animation_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(race_option_animation_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME race_option_animation COMMAND race_option_animation_tests)
set_tests_properties(race_option_animation PROPERTIES LABELS unit)

add_executable(asset_table_installation_tests
    asset_table_installation_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/install_track_event_data.c)
target_include_directories(asset_table_installation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(asset_table_installation_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(asset_table_installation_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(asset_table_installation_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME asset_table_installation COMMAND asset_table_installation_tests)
set_tests_properties(asset_table_installation PROPERTIES LABELS unit)

add_executable(course_event_runtime_tests
    course_event_runtime_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/course_event_runtime.c)
target_include_directories(course_event_runtime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(course_event_runtime_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(course_event_runtime_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(course_event_runtime_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME course_event_runtime COMMAND course_event_runtime_tests)
set_tests_properties(course_event_runtime PROPERTIES LABELS unit)

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

set_tests_properties(record_defaults PROPERTIES LABELS unit)

add_executable(round_screen_rules_tests
    round_screen_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/round_screen_rules.c)
target_include_directories(round_screen_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(round_screen_rules_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME round_screen_rules COMMAND round_screen_rules_tests)
set_tests_properties(round_screen_rules PROPERTIES LABELS unit)

add_executable(race_end_rules_tests
    race_end_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/race_end_rules.c)
target_include_directories(race_end_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(race_end_rules_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(race_end_rules_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(race_end_rules_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(race_end_rules_tests PRIVATE
        -Wall -Wextra -Wno-deprecated-non-prototype -Werror)
endif()
add_test(NAME race_end_rules COMMAND race_end_rules_tests)
set_tests_properties(race_end_rules PROPERTIES LABELS unit)

add_executable(result_screen_rules_tests
    result_screen_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/result_screen_rules.c)
target_include_directories(result_screen_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(result_screen_rules_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(result_screen_rules_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(result_screen_rules_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(result_screen_rules_tests PRIVATE
        -Wall -Wextra -Wno-deprecated-non-prototype -Werror)
endif()
add_test(NAME result_screen_rules COMMAND result_screen_rules_tests)
set_tests_properties(result_screen_rules PROPERTIES LABELS unit)

add_executable(race_scene_rules_tests
    race_scene_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/race_scene_rules.c)
target_include_directories(race_scene_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(race_scene_rules_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME race_scene_rules COMMAND race_scene_rules_tests)
set_tests_properties(race_scene_rules PROPERTIES LABELS unit)

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

if(TARGET rage-host-state)
add_executable(save_defaults_tests
    save_defaults_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_defaults.c)
target_include_directories(save_defaults_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(save_defaults_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(save_defaults_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(save_defaults_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(save_defaults_tests PRIVATE rage-host-state psyz)
add_test(NAME save_defaults COMMAND save_defaults_tests)
set_tests_properties(save_defaults PROPERTIES LABELS unit)
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
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/lap_and_finish.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/time_conversion.c)
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
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_select_control.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_track.c)
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
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_select_transition.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/bgm_track.c)
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

add_executable(record_name_entry_tests
    record_name_entry_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/record_name_entry.c)
target_include_directories(record_name_entry_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(record_name_entry_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(record_name_entry_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(record_name_entry_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME record_name_entry COMMAND record_name_entry_tests)
set_tests_properties(record_name_entry PROPERTIES LABELS unit)

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
