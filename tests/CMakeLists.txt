add_executable(game_logic_tests
    game_logic_tests.c
    ../src/port/input_config.c
    ../src/port/port_config.c
    ../src/port/runtime_config.c
    ../src/port/game_diagnostics.c
    ../src/port/platform_paths.c
)
target_link_libraries(game_logic_tests PRIVATE rage-sim)

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
add_test(NAME architecture_boundaries COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_architecture_boundaries.py
    ${CMAKE_SOURCE_DIR})
add_test(NAME native_car_names COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_car_names.py
    ${CMAKE_SOURCE_DIR})

add_executable(team_logo_tests
    team_logo_tests.c
    ../src/main/PAL/main/menu/team_logo.c)
target_include_directories(team_logo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(team_logo_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(team_logo_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(team_logo_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(team_logo_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
add_test(NAME team_logo COMMAND team_logo_tests)

add_executable(record_defaults_tests
    record_defaults_tests.c
    ../src/main/PAL/main/race/records.c)
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

add_executable(hud_config_tests
    hud_config_tests.c
    ../src/port/hud_config.c)
target_include_directories(hud_config_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hud_config_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME hud_config COMMAND hud_config_tests)

add_executable(chase_camera_tests
    chase_camera_tests.c
    ../src/port/chase_camera.c)
target_include_directories(chase_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(chase_camera_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME chase_camera COMMAND chase_camera_tests)

add_executable(content_options_tests
    content_options_tests.c
    ../src/port/content_options.c)
target_include_directories(content_options_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(content_options_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME content_options COMMAND content_options_tests)

add_executable(input_device_select_tests
    input_device_select_tests.c
    ../src/port/input_device_select.c)
target_include_directories(input_device_select_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(input_device_select_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME input_device_select COMMAND input_device_select_tests)

add_executable(rmesh_tests rmesh_tests.c)
target_link_libraries(rmesh_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rmesh_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME rmesh COMMAND rmesh_tests)

add_executable(rmesh_index_tests rmesh_index_tests.c)
target_link_libraries(rmesh_index_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rmesh_index_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME rmesh_index COMMAND rmesh_index_tests)

add_executable(asset_id_tests asset_id_tests.c)
target_link_libraries(asset_id_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(asset_id_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME asset_id COMMAND asset_id_tests)

add_executable(car_paint_tests car_paint_tests.c)
target_link_libraries(car_paint_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_paint_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME car_paint COMMAND car_paint_tests)

add_executable(texture_mipmap_tests texture_mipmap_tests.c)
target_link_libraries(texture_mipmap_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(texture_mipmap_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME texture_mipmap COMMAND texture_mipmap_tests)

add_executable(mod_manifest_tests mod_manifest_tests.c)
target_link_libraries(mod_manifest_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mod_manifest_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME mod_manifest COMMAND mod_manifest_tests)

add_executable(mod_asset_fallback_tests
    mod_asset_fallback_tests.c
    ../src/port/mod_assets.c)
target_include_directories(mod_asset_fallback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mod_asset_fallback_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME mod_asset_fallback COMMAND mod_asset_fallback_tests)

add_executable(render_material_tests render_material_tests.c)
target_link_libraries(render_material_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_material_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME render_material COMMAND render_material_tests)

add_executable(rmesh_cache_tests rmesh_cache_tests.c)
target_link_libraries(rmesh_cache_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rmesh_cache_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME rmesh_cache COMMAND rmesh_cache_tests)

add_test(NAME asset_gltf_export
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/assetbrowser/test_gltf.py)
add_test(NAME asset_sky_export
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/assetbrowser/test_sky.py)
add_test(NAME native_sky_projection
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_sky_projection.py)

add_executable(render_world_tests render_world_tests.c)
target_link_libraries(render_world_tests PRIVATE rage-render-world)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_world_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME render_world COMMAND render_world_tests)

add_executable(render_world_snapshot_tests render_world_snapshot_tests.c)
target_link_libraries(render_world_snapshot_tests PRIVATE rage-render-world)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_world_snapshot_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME render_world_snapshot COMMAND render_world_snapshot_tests)

add_executable(track_asset_identity_tests
    track_asset_identity_tests.c
    ../src/port/track_asset_identity.c)
target_include_directories(track_asset_identity_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_asset_identity_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME track_asset_identity COMMAND track_asset_identity_tests)

add_executable(track_lighting_tests
    track_lighting_tests.c
    ../src/port/track_lighting.c)
target_include_directories(track_lighting_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_lighting_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME track_lighting COMMAND track_lighting_tests)

add_executable(course_coordinate_tests course_coordinate_tests.c)
target_include_directories(course_coordinate_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/port)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(course_coordinate_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME course_coordinate COMMAND course_coordinate_tests)

add_executable(render_mesh_build_tests render_mesh_build_tests.c)
target_link_libraries(render_mesh_build_tests PRIVATE rage-render-world)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_mesh_build_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME render_mesh_build COMMAND render_mesh_build_tests)

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

add_executable(release_package_tests release_package_tests.c)
if(MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(release_package_tests PRIVATE
        /clang:-Wall /clang:-Wextra /clang:-Werror
        /clang:-Wno-unsafe-buffer-usage)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(release_package_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME release_package COMMAND release_package_tests ${CMAKE_SOURCE_DIR})

add_test(NAME host_state_abi COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_host_state_abi.py ${CMAKE_SOURCE_DIR})

add_test(NAME shipped_config COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_shipped_config.py ${CMAKE_SOURCE_DIR})

if(APPLE AND TARGET rage-racer)
    add_test(NAME macos_build_launcher COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_macos_build_launcher.py
        ${CMAKE_BINARY_DIR}/rage-racer
        $<TARGET_FILE:rage-racer>)
    set_tests_properties(macos_build_launcher PROPERTIES LABELS portable)
endif()

add_test(NAME car_prices COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_car_prices.py ${CMAKE_SOURCE_DIR})

# Test timeouts are wall-clock, and most of these run the game at its own pace
# rather than as fast as the machine can. race_cdda waits out an audio track and
# needs 66 seconds of real time; its limit was 45, so it failed whenever anything
# else was running. Every limit here is three times the time its test takes on an
# idle machine, which is enough that a busy one does not turn a passing test into
# a failing one. They still catch a hang, which is what they are for.
add_test(
    NAME player_car_interior_aliases
    COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_player_car_aliases.py
        ${CMAKE_SOURCE_DIR}
)

# Visual-comparison helpers remain optional developer tools. Do not register
# them as failing tests on hosts that intentionally ship only the C/Python
# toolchain used by the port and release jobs.
find_program(RAGE_RUBY_EXECUTABLE ruby)
if(RAGE_RUBY_EXECUTABLE)
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
endif()

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
    target_link_libraries(axis_curve_tests PRIVATE m)
endif()
add_executable(track_math_tests
    track_math_tests.c
    ../src/main/PAL/main/render/track_textures.c)
target_include_directories(track_math_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(track_math_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(track_math_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(track_math_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
target_link_libraries(track_math_tests PRIVATE psyz)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_math_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
add_test(NAME track_math COMMAND track_math_tests)

if(TARGET rage-host-state)
add_executable(track_camera_tests
    track_camera_tests.c
    ../src/main/PAL/main/track/update_camera.c
    ../src/main/PAL/main/render/rot_matrix.c
    ../src/main/PAL/main/render/gte_matrix.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(track_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(track_camera_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(track_camera_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(track_camera_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
target_link_libraries(track_camera_tests PRIVATE psyz)
if(NOT MSVC)
    target_compile_options(track_camera_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
add_test(NAME track_camera COMMAND track_camera_tests)
set_tests_properties(track_camera PROPERTIES LABELS portable)

# The three AI walks over the course's own crest, speed and racing-line tables.
add_executable(car_ai_tests
    car_ai_tests.c
    ../src/main/PAL/main/car/car_ai.c)
target_include_directories(car_ai_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_ai_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_ai_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_ai_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_ai_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_ai_tests PRIVATE psyz)
add_test(NAME car_ai COMMAND car_ai_tests)
set_tests_properties(car_ai PROPERTIES LABELS portable)

# The prize screen's arithmetic, which is what a race leaves behind.
add_executable(prize_screen_tests
    prize_screen_tests.c
    ../src/main/PAL/main/menu/prize_money_screen.c)
target_include_directories(prize_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(prize_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(prize_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(prize_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(prize_screen_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
target_link_libraries(prize_screen_tests PRIVATE psyz)
add_test(NAME prize_screen COMMAND prize_screen_tests)
set_tests_properties(prize_screen PROPERTIES LABELS portable)

# The drivetrain branches a smoke run never reaches: tall gears and mid-shift.
add_executable(drivetrain_tests
    drivetrain_tests.c
    ../src/main/PAL/main/car/update_car_drivetrain.c)
target_include_directories(drivetrain_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(drivetrain_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(drivetrain_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(drivetrain_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(drivetrain_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
target_link_libraries(drivetrain_tests PRIVATE psyz)
add_test(NAME drivetrain COMMAND drivetrain_tests)
set_tests_properties(drivetrain PROPERTIES LABELS portable)

# The memory card menu, swept across its whole state space.
add_executable(memory_card_menu_tests
    memory_card_menu_tests.c
    ../src/main/PAL/main/save/update_memory_card_menu.c)
target_include_directories(memory_card_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(memory_card_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(memory_card_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(memory_card_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(memory_card_menu_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(memory_card_menu_tests PRIVATE psyz)
add_test(NAME memory_card_menu COMMAND memory_card_menu_tests)
set_tests_properties(memory_card_menu PROPERTIES LABELS portable)

# Where a car lands on the track, swept over a course built for the purpose.
if(TARGET rage-host-state)
add_executable(car_track_state_tests
    car_track_state_tests.c
    ../src/main/PAL/main/car/car_track_state.c
    ../src/main/PAL/main/car/angle_math.c
    ../src/main/PAL/main/render/rot_matrix.c
    ../src/main/PAL/main/render/gte_matrix.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(car_track_state_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_track_state_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_track_state_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_track_state_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_track_state_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_track_state_tests PRIVATE psyz)
add_test(NAME car_track_state COMMAND car_track_state_tests)
set_tests_properties(car_track_state PROPERTIES LABELS portable)

# The gearbox, on its own rather than through a whole race.
add_executable(gear_shift_tests
    gear_shift_tests.c
    ../src/main/PAL/main/car/gear_shift.c)
target_include_directories(gear_shift_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(gear_shift_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(gear_shift_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(gear_shift_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(gear_shift_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
target_link_libraries(gear_shift_tests PRIVATE psyz)
add_test(NAME gear_shift COMMAND gear_shift_tests)
set_tests_properties(gear_shift PROPERTIES LABELS portable)

# The logo editor's two halves, swept across the buttons that act on them.
add_executable(team_logo_editor_tests
    team_logo_editor_tests.c
    ../src/main/PAL/main/menu/team_logo_editor.c)
target_include_directories(team_logo_editor_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(team_logo_editor_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(team_logo_editor_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(team_logo_editor_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(team_logo_editor_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(team_logo_editor_tests PRIVATE psyz)
add_test(NAME team_logo_editor COMMAND team_logo_editor_tests)
set_tests_properties(team_logo_editor PROPERTIES LABELS portable)

add_executable(draw_team_logo_canvas_tests
    draw_team_logo_canvas_tests.c
    ../src/main/PAL/main/menu/draw_team_logo_canvas.c)
target_include_directories(draw_team_logo_canvas_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(draw_team_logo_canvas_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(draw_team_logo_canvas_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(draw_team_logo_canvas_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(draw_team_logo_canvas_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(draw_team_logo_canvas_tests PRIVATE psyz)
add_test(NAME draw_team_logo_canvas COMMAND draw_team_logo_canvas_tests)
set_tests_properties(draw_team_logo_canvas PROPERTIES LABELS portable)

add_executable(car_select_screen_tests
    car_select_screen_tests.c
    ../src/main/PAL/main/menu/car_select_screen.c
    ../src/main/PAL/main/menu/menu_turntable.c)
target_include_directories(car_select_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_select_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_select_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_select_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_select_screen_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_select_screen_tests PRIVATE psyz)
add_test(NAME car_select_screen COMMAND car_select_screen_tests)
set_tests_properties(car_select_screen PROPERTIES LABELS portable)

add_executable(car_shop_screen_tests
    car_shop_screen_tests.c
    ../src/main/PAL/main/menu/car_shop_screen.c
    ../src/main/PAL/main/menu/shop_prompt.c
    ../src/main/PAL/main/menu/menu_turntable.c)
target_include_directories(car_shop_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_shop_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_shop_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_shop_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_shop_screen_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_shop_screen_tests PRIVATE psyz)
add_test(NAME car_shop_screen COMMAND car_shop_screen_tests)
set_tests_properties(car_shop_screen PROPERTIES LABELS portable)

add_executable(engineer_shop_screen_tests
    engineer_shop_screen_tests.c
    ../src/main/PAL/main/menu/engineer_shop_screen.c
    ../src/main/PAL/main/menu/shop_prompt.c)
target_include_directories(engineer_shop_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(engineer_shop_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(engineer_shop_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(engineer_shop_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(engineer_shop_screen_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(engineer_shop_screen_tests PRIVATE psyz)
add_test(NAME engineer_shop_screen COMMAND engineer_shop_screen_tests)
set_tests_properties(engineer_shop_screen PROPERTIES LABELS portable)

add_executable(lap_and_finish_tests
    lap_and_finish_tests.c
    ../src/main/PAL/main/race/lap_and_finish.c)
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
set_tests_properties(lap_and_finish PROPERTIES LABELS portable)

add_executable(rival_collision_tests
    rival_collision_tests.c
    ../src/main/PAL/main/car/rival_collision.c
    ../src/main/PAL/main/car/collision_quads.c)
target_include_directories(rival_collision_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(rival_collision_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(rival_collision_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(rival_collision_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(rival_collision_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(rival_collision_tests PRIVATE psyz)
add_test(NAME rival_collision COMMAND rival_collision_tests)
set_tests_properties(rival_collision PROPERTIES LABELS portable)
endif()
endif()

set_tests_properties(track_math PROPERTIES LABELS portable)

add_test(NAME axis_curve COMMAND axis_curve_tests)
set_tests_properties(axis_curve PROPERTIES LABELS portable)

# The port's half of the NeGcon (NegconTwist) against the game's half
# (UpdatePadState), which is the only place a calibration fault shows.
if(TARGET rage-host-state)
    add_executable(negcon_calibration_tests
        negcon_calibration_tests.c
        ../src/port/axis_curve.c
        ../src/main/PAL/main/pad/init_pad.c
        $<TARGET_OBJECTS:rage-host-state>
    )
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
    if(RAGE_ENABLE_SANITIZERS AND
       CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
        target_link_options(negcon_calibration_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
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
    set_tests_properties(mod_tools PROPERTIES TIMEOUT 180)
endif()

set(RAGE_PORTABLE_TESTS
    game_logic course_index native_geometry_interpolation
    release_package host_state_abi player_car_interior_aliases car_prices shipped_config
    mod_tools axis_curve timing_restore
)
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
    set_tests_properties(boot_frame PROPERTIES TIMEOUT 60)
    add_test(
        NAME title_page_stability
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_title_page_stability.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(title_page_stability PROPERTIES TIMEOUT 60)
    add_test(
        NAME scenario_control
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scenario_control.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(scenario_control PROPERTIES TIMEOUT 150)
    add_test(
        NAME direct_boot_car_setup
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_direct_boot_car_setup.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(direct_boot_car_setup PROPERTIES TIMEOUT 105)
    add_test(
        NAME scenario_after_finish
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scenario_after_finish.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(scenario_after_finish PROPERTIES TIMEOUT 210)
    add_test(
        NAME runtime_config
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_runtime_config.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(runtime_config PROPERTIES TIMEOUT 180)
    add_test(
        NAME renderer_toggle_cycles
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_renderer_toggle_cycles.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(renderer_toggle_cycles PROPERTIES TIMEOUT 195)
    add_test(
        NAME course_matrix
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_course_matrix.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(course_matrix PROPERTIES TIMEOUT 1080)
    add_test(
        NAME track_start
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_track_start.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(track_start PROPERTIES TIMEOUT 165)
    add_test(
        NAME diagnostic_log
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_diagnostic_log.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(diagnostic_log PROPERTIES TIMEOUT 60)
    add_test(NAME retire_camera COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_retire_camera.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
    set_tests_properties(retire_camera PROPERTIES TIMEOUT 165)
    add_test(NAME mirrored_track COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirrored_track.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
    set_tests_properties(mirrored_track PROPERTIES TIMEOUT 360)
    add_test(
        NAME keyboard_input
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_keyboard_input.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(keyboard_input PROPERTIES TIMEOUT 60)
    add_test(
        NAME window_size
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_window_size.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
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
        set_tests_properties(window_lifecycle PROPERTIES TIMEOUT 195)
    endif()
    add_test(
        NAME initial_state
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_initial_state.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(initial_state PROPERTIES TIMEOUT 60)
    add_test(
        NAME fmv_frame_layout
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    # Skips rather than fails when there is no disc to read the movies
    # or the music from; see tests/disc_image.py.
    set_tests_properties(fmv_frame_layout PROPERTIES TIMEOUT 75
        SKIP_RETURN_CODE 77)
    add_test(
        NAME fmv_without_ffmpeg
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_without_ffmpeg.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(fmv_without_ffmpeg PROPERTIES TIMEOUT 90
        SKIP_RETURN_CODE 77)
    add_test(
        NAME fmv_pacing
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_pacing.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(fmv_pacing PROPERTIES TIMEOUT 600
        SKIP_RETURN_CODE 77)
    add_test(
        NAME prologue_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_prologue_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(prologue_frame PROPERTIES TIMEOUT 90)
    add_test(
        NAME japanese_prologue_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_prologue_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
            japanese
    )
    set_tests_properties(japanese_prologue_frame PROPERTIES TIMEOUT 90)

    add_test(
        NAME scene_capture
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scene_capture.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(scene_capture PROPERTIES TIMEOUT 360)
    add_test(
        NAME modern_feature_frames
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_modern_feature_frames.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(modern_feature_frames PROPERTIES TIMEOUT 360)

    add_test(
        NAME pegase_cabin
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_pegase_cabin.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(pegase_cabin PROPERTIES TIMEOUT 180)

    # The sky, which nothing watched: the visual references are captured from
    # menus and the intro camera, where its horizon band and skirt are not
    # drawn at all.
    add_test(
        NAME sky_frames
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_sky_frames.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(sky_frames PROPERTIES TIMEOUT 600)

    add_test(
        NAME native_render_world
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_render_world.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(native_render_world PROPERTIES TIMEOUT 120)

    add_test(
        NAME modern_disc_import_bootstrap
        COMMAND $<TARGET_FILE:rage-racer-smoke>
            --set video.renderer=modern
            --set modern.assets=disc
            --set run.frames=1
    )
    set_tests_properties(modern_disc_import_bootstrap PROPERTIES
        TIMEOUT 20
        ENVIRONMENT "SDL_AUDIODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "native assets generated by C importer")

    add_test(
        NAME grand_prix_terrain_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_terrain_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(grand_prix_terrain_frame PROPERTIES TIMEOUT 120)

    add_test(
        NAME race_start
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_start.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(race_start PROPERTIES TIMEOUT 150)
    add_test(
        NAME race_restart
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_restart.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(race_restart PROPERTIES TIMEOUT 195)

    add_test(
        NAME grand_prix_results
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_results.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(grand_prix_results PROPERTIES TIMEOUT 225
        SKIP_RETURN_CODE 77)

    add_test(
        NAME grand_prix_exit
        COMMAND ruby
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_exit.rb
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(grand_prix_exit PROPERTIES TIMEOUT 225)

    add_test(
        NAME mirror_entry
        COMMAND ruby
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirror_entry.rb
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(mirror_entry PROPERTIES TIMEOUT 150)
    add_test(NAME mirror_cars COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirror_cars.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
    set_tests_properties(mirror_cars PROPERTIES TIMEOUT 360)

    add_test(
        NAME menu_sweep
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_menu_sweep.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(menu_sweep PROPERTIES TIMEOUT 225)

    add_test(
        NAME trophy_view
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_trophy_view.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(trophy_view PROPERTIES TIMEOUT 165)

    add_test(
        NAME waterfall
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_waterfall.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(waterfall PROPERTIES TIMEOUT 180)

    add_test(
        NAME time_attack_records
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_time_attack_records.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(time_attack_records PROPERTIES TIMEOUT 225)

    add_test(
        NAME save_roundtrip
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_save_roundtrip.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
            $<TARGET_FILE:rage-save-generator>
    )
    set_tests_properties(save_roundtrip PROPERTIES TIMEOUT 270)

    add_test(
        NAME audio_output
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_audio_output.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(audio_output PROPERTIES TIMEOUT 90)

    add_test(
        NAME race_cdda
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_cdda.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(race_cdda PROPERTIES TIMEOUT 135
        SKIP_RETURN_CODE 77)
    add_test(
        NAME engine_audio
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_engine_audio.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(engine_audio PROPERTIES TIMEOUT 210)
endif()
