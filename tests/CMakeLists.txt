add_executable(game_logic_tests
    game_logic_tests.c
    ../src/main/PAL/main/race/time_conversion.c
    ../src/main/PAL/main/random/random15.c
    ../src/main/PAL/main/car/angle_math.c
    ../src/main/PAL/main/track/blend_angle.c
    ../src/main/PAL/main/track/interpolate_track_angle.c
    ../src/main/PAL/main/track/lerp_color_channel.c
    ../src/port/input_config.c
    ../src/port/port_config.c
    ../src/port/runtime_config.c
    ../src/port/game_diagnostics.c
    ../src/port/platform_paths.c
)

target_include_directories(game_logic_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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

add_executable(course_index_tests course_index_tests.c)
target_include_directories(course_index_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(course_index_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME course_index COMMAND course_index_tests)

add_executable(native_geometry_interpolation_tests
    native_geometry_interpolation_tests.c
    ../src/port/native_geometry_interpolation.c)
target_include_directories(native_geometry_interpolation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
target_link_libraries(native_geometry_interpolation_tests PRIVATE psyz)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(native_geometry_interpolation_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
if(RAGE_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(native_geometry_interpolation_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(native_geometry_interpolation_tests PRIVATE
        -fsanitize=address,undefined)
endif()
add_test(NAME native_geometry_interpolation
         COMMAND native_geometry_interpolation_tests)

add_test(NAME release_package COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_release_package.py ${CMAKE_SOURCE_DIR})

add_test(NAME host_state_abi COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_host_state_abi.py ${CMAKE_SOURCE_DIR})

add_test(NAME shipped_config COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_shipped_config.py ${CMAKE_SOURCE_DIR})

add_test(NAME car_prices COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_car_prices.py ${CMAKE_SOURCE_DIR})

add_test(
    NAME player_car_interior_aliases
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_player_car_aliases.rb
        ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME race_progress_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_progress_layout.rb
        ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME sequence_state_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_sequence_state_layout.rb
        ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME native_ui_script_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_ui_script_layout.rb
        ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME native_frame_offsets
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_frame_offsets.rb
        ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME frame_environment_aliases
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_frame_environment_aliases.rb
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/frontend.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rear_view_mirror.c
        ${CMAKE_SOURCE_DIR}/src/port/host_state.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/tachometer_needle.c
)

add_test(
    NAME audio_state_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_audio_state_layout.rb
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/sound_runtime.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/loaded_audio_voices.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_audio.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/records.c
        ${CMAKE_SOURCE_DIR}/src/port/host_state.c
        ${CMAKE_SOURCE_DIR}/include/game/track.h
)

add_test(
    NAME visual_batch_refinement
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_batch_refinement.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_batch.rb
)

add_test(
    NAME visual_run
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_run.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_run.rb
)

add_test(
    NAME capture_sync
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_capture_sync.rb
        ${CMAKE_SOURCE_DIR}/src/port/main_smoke.c
)

add_test(
    NAME visual_budget
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_budget.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_budget.rb
)

add_test(
    NAME visual_triage
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_triage.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_triage.rb
)

add_test(
    NAME gpu_digest_compare
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_gpu_digest_compare.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_gpu_digest_compare.rb
)

add_test(
    NAME gp0_compare
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_compare.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_gp0_compare.rb
)

add_test(
    NAME gp0_bundle
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_bundle.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_gp0_bundle.rb
)
add_test(
    NAME gp0_replay
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_replay.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_gp0_replay.c
)
add_test(
    NAME gp0_raster_compare
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_raster_compare.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_gp0_raster_compare.rb
)
add_test(
    NAME vram_delta
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_vram_delta.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_vram_delta.rb
)
add_test(
    NAME visible_cells_compare
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_visible_cells_compare.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_visible_cells_compare.rb
)

add_test(
    NAME input_replay
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_input_replay.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_input_replay.rb
)

if(TARGET rage-gte-replay)
    add_test(NAME gte_replay_usage COMMAND rage-gte-replay)
    set_tests_properties(gte_replay_usage PROPERTIES WILL_FAIL TRUE)
endif()

# These tests need neither a staged disc image nor a windowing session and are
# therefore safe in every release-packaging job. Runtime tests below exercise
# the real PAL data in local/pre-release validation instead.
add_executable(timing_restore_tests timing_restore_tests.c ../src/port/timing_restore.c)
target_include_directories(timing_restore_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/port ${CMAKE_CURRENT_SOURCE_DIR}/../src/port/include)
add_test(NAME timing_restore COMMAND timing_restore_tests)
set_tests_properties(timing_restore PROPERTIES LABELS portable)

add_executable(axis_curve_tests axis_curve_tests.c ../src/port/axis_curve.c)
target_include_directories(axis_curve_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/port)
if(NOT MSVC)
    target_compile_options(axis_curve_tests PRIVATE -Wall -Wextra)
endif()
add_test(NAME axis_curve COMMAND axis_curve_tests)
set_tests_properties(axis_curve PROPERTIES LABELS portable)

# The port's half of the NeGcon (RageNegconTwist) against the game's half
# (UpdatePadState), which is the only place a calibration fault shows.
if(TARGET rage-host-state)
    add_executable(negcon_calibration_tests
        negcon_calibration_tests.c
        ../src/port/axis_curve.c
        ../src/main/PAL/main/pad/init_pad.c
        $<TARGET_OBJECTS:rage-host-state>
    )
    target_compile_definitions(negcon_calibration_tests PRIVATE RAGE_HOST_PORT=1)
    target_include_directories(negcon_calibration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/port
        ${CMAKE_SOURCE_DIR}/src/port/include)
    if(NOT WIN32)
        target_compile_options(negcon_calibration_tests PRIVATE
            "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
            "-iquote${CMAKE_SOURCE_DIR}/include")
    endif()
    target_link_libraries(negcon_calibration_tests PRIVATE psyz)
    add_test(NAME negcon_calibration COMMAND negcon_calibration_tests)
    set_tests_properties(negcon_calibration PROPERTIES LABELS portable)
endif()

if(TARGET rage-extract AND TARGET rage-pack)
    add_test(
        NAME mod_tools
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_mod_tools.py
            $<TARGET_FILE:rage-extract>
            $<TARGET_FILE:rage-pack>
    )
    set_tests_properties(mod_tools PROPERTIES TIMEOUT 60)
endif()

set(RAGE_PORTABLE_TESTS
    game_logic course_index native_geometry_interpolation
    release_package host_state_abi player_car_interior_aliases car_prices shipped_config
    race_progress_layout sequence_state_layout native_ui_script_layout
    native_frame_offsets frame_environment_aliases audio_state_layout
    mod_tools axis_curve timing_restore
    visual_run capture_sync visual_budget visual_triage
    gpu_digest_compare gp0_compare gp0_bundle gp0_replay gp0_raster_compare
    vram_delta visible_cells_compare input_replay)
if(TARGET rage-gte-replay)
    list(APPEND RAGE_PORTABLE_TESTS gte_replay_usage)
endif()
set_tests_properties(${RAGE_PORTABLE_TESTS} PROPERTIES LABELS portable)

if(TARGET rage-racer-smoke)
    add_test(
        NAME boot_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_boot_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(boot_frame PROPERTIES TIMEOUT 20)
    add_test(
        NAME title_page_stability
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_title_page_stability.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(title_page_stability PROPERTIES TIMEOUT 20)
    add_test(
        NAME scenario_control
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scenario_control.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(scenario_control PROPERTIES TIMEOUT 50)
    add_test(
        NAME scenario_after_finish
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scenario_after_finish.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(scenario_after_finish PROPERTIES TIMEOUT 70)
    add_test(
        NAME runtime_config
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_runtime_config.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(runtime_config PROPERTIES TIMEOUT 60)
    add_test(
        NAME renderer_toggle_cycles
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_renderer_toggle_cycles.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(renderer_toggle_cycles PROPERTIES TIMEOUT 65)
    add_test(
        NAME course_matrix
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_course_matrix.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(course_matrix PROPERTIES TIMEOUT 360)
    add_test(
        NAME track_start
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_track_start.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(track_start PROPERTIES TIMEOUT 55)
    add_test(
        NAME diagnostic_log
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_diagnostic_log.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(diagnostic_log PROPERTIES TIMEOUT 20)
    add_test(NAME retire_camera COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_retire_camera.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
    set_tests_properties(retire_camera PROPERTIES TIMEOUT 55)
    add_test(NAME mirrored_track COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirrored_track.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
    set_tests_properties(mirrored_track PROPERTIES TIMEOUT 120)
    add_test(
        NAME keyboard_input
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_keyboard_input.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(keyboard_input PROPERTIES TIMEOUT 20)
    add_test(
        NAME window_size
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_window_size.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(window_size PROPERTIES TIMEOUT 20)
    find_program(RAGE_XDOTOOL xdotool)
    if(RAGE_XDOTOOL)
        add_test(
            NAME window_lifecycle
            COMMAND python3
                ${CMAKE_CURRENT_SOURCE_DIR}/verify_window_lifecycle.py
                $<TARGET_FILE:rage-racer-smoke>
                ${CMAKE_SOURCE_DIR}
        )
        set_tests_properties(window_lifecycle PROPERTIES TIMEOUT 65)
    endif()
    add_test(
        NAME initial_state
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_initial_state.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(initial_state PROPERTIES TIMEOUT 20)
    add_test(
        NAME fmv_frame_layout
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(fmv_frame_layout PROPERTIES TIMEOUT 25)
    add_test(
        NAME fmv_without_ffmpeg
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_without_ffmpeg.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(fmv_without_ffmpeg PROPERTIES TIMEOUT 30)
    add_test(
        NAME fmv_pacing
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_pacing.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(fmv_pacing PROPERTIES TIMEOUT 200)
    add_test(
        NAME prologue_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_prologue_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(prologue_frame PROPERTIES TIMEOUT 30)

    add_test(
        NAME scene_capture
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scene_capture.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(scene_capture PROPERTIES TIMEOUT 120)
    add_test(
        NAME modern_feature_frames
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_modern_feature_frames.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(modern_feature_frames PROPERTIES TIMEOUT 120)

    add_test(
        NAME grand_prix_terrain_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_terrain_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(grand_prix_terrain_frame PROPERTIES TIMEOUT 40)

    add_test(
        NAME race_start
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_start.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(race_start PROPERTIES TIMEOUT 50)
    add_test(
        NAME race_restart
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_restart.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(race_restart PROPERTIES TIMEOUT 65)

    add_test(
        NAME grand_prix_results
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_results.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(grand_prix_results PROPERTIES TIMEOUT 75)

    add_test(
        NAME grand_prix_exit
        COMMAND ruby
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_exit.rb
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(grand_prix_exit PROPERTIES TIMEOUT 75)

    add_test(
        NAME mirror_entry
        COMMAND ruby
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirror_entry.rb
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(mirror_entry PROPERTIES TIMEOUT 50)
    add_test(NAME mirror_cars COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirror_cars.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
    set_tests_properties(mirror_cars PROPERTIES TIMEOUT 120)

    add_test(
        NAME menu_sweep
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_menu_sweep.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(menu_sweep PROPERTIES TIMEOUT 75)

    add_test(
        NAME trophy_view
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_trophy_view.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(trophy_view PROPERTIES TIMEOUT 55)

    add_test(
        NAME waterfall
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_waterfall.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(waterfall PROPERTIES TIMEOUT 60)

    add_test(
        NAME time_attack_records
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_time_attack_records.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(time_attack_records PROPERTIES TIMEOUT 75)

    add_test(
        NAME save_roundtrip
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_save_roundtrip.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
            $<TARGET_FILE:rage-save-generator>
    )
    set_tests_properties(save_roundtrip PROPERTIES TIMEOUT 90)

    add_test(
        NAME audio_output
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_audio_output.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(audio_output PROPERTIES TIMEOUT 30)

    add_test(
        NAME race_cdda
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_cdda.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(race_cdda PROPERTIES TIMEOUT 45)
    add_test(
        NAME engine_audio
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_engine_audio.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(engine_audio PROPERTIES TIMEOUT 70)
endif()
