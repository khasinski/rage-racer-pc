add_executable(game_logic_tests
    game_logic_tests.c
    ../src/main/PAL/main/race/time_conversion.c
    ../src/main/PAL/main/random/random15.c
    ../src/main/PAL/main/car/angle_math.c
    ../src/main/PAL/main/track/blend_angle.c
    ../src/main/PAL/main/track/interpolate_track_angle.c
    ../src/main/PAL/main/track/lerp_color_channel.c
    ../src/port/input_config.c
)

target_include_directories(game_logic_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(game_logic_tests PRIVATE -Wall -Wextra -Werror)
endif()

add_test(NAME game_logic COMMAND game_logic_tests)

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
        NAME prologue_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_prologue_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(prologue_frame PROPERTIES TIMEOUT 30)

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

    add_test(
        NAME menu_sweep
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_menu_sweep.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(menu_sweep PROPERTIES TIMEOUT 75)

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
endif()
