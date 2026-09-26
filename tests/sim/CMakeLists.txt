# The same narrow simulation tests, without client, SDL or PsyZ targets.
foreach(test IN ITEMS
    car/engine_sound car/rival_contact car/rival_traffic car/rival_init car/car_lateral_offset car/steer_car_along_route car/rival_place car/rival_finish car/rival_car_movement car/rival_line car/rival_acceleration car/rival_target car/rival_speed_key car/rival_config car/shift_pitch car/car_asset car/track_data car/track_events car/track_zone car/track_route car/trig car/car_shift car/driver_input car/drivetrain_step car/airborne_step
    car/crest_step car/jump_step car/track_contact_step car/contact_response_step
    car/collision_response car/car_contact car/route_interpolation car/route_steering
    car/driver_contact car/driver_init car/driver_step car/driver_field car/catalog_parse car/client_driver car/player_engine_presentation
    race/race_lap race/race_sim race/race_mixed race/race_input race/race_lifecycle race/race_setup race/race_position race/race_time)
    get_filename_component(name "${test}" NAME)
    add_executable(${name}_tests ${CMAKE_SOURCE_DIR}/tests/${test}_tests.c)
    target_include_directories(${name}_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(${name}_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
    target_link_libraries(${name}_tests PRIVATE rage-sim)
    add_test(NAME ${name} COMMAND ${name}_tests)
    set_tests_properties(${name} PROPERTIES LABELS unit)
endforeach()

target_sources(player_engine_presentation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_engine_presentation.c)

target_sources(client_driver_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_player_car.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c)

include(${CMAKE_CURRENT_SOURCE_DIR}/retail_data.cmake)

foreach(test IN ITEMS race_data race_disc)
    add_executable(${test}_tests ${CMAKE_SOURCE_DIR}/tests/car/${test}_tests.c)
    target_link_libraries(${test}_tests PRIVATE rage-data)
    target_compile_options(${test}_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
    add_test(NAME ${test} COMMAND ${test}_tests)
    set_tests_properties(${test} PROPERTIES LABELS unit)
endforeach()

# Presentation boundary test: no race physics or platform library required.
add_executable(driver_cues_tests ${CMAKE_SOURCE_DIR}/tests/car/driver_cues_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_jump_audio.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_contact_audio.c)
target_include_directories(driver_cues_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(driver_cues_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(driver_cues_tests PRIVATE rage-sim)
add_test(NAME driver_cues COMMAND driver_cues_tests)
set_tests_properties(driver_cues PROPERTIES LABELS unit)

# Per-car rendering math also runs without the game or a GPU.
add_executable(car_parts_tests ${CMAKE_SOURCE_DIR}/tests/render/car_parts_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/car_parts.c)
target_include_directories(car_parts_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port ${CMAKE_SOURCE_DIR}/include)
target_compile_options(car_parts_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(car_parts_tests PRIVATE rage-render-world rage-sim)
add_test(NAME car_parts COMMAND car_parts_tests)
set_tests_properties(car_parts PROPERTIES LABELS unit)
add_executable(race_view_tests ${CMAKE_SOURCE_DIR}/tests/render/race_view_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/race_view.c)
target_include_directories(race_view_tests PRIVATE ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_options(race_view_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_sources(race_view_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port/car_parts.c)
target_link_libraries(race_view_tests PRIVATE rage-data rage-render-world)
add_test(NAME race_view COMMAND race_view_tests)
set_tests_properties(race_view PROPERTIES LABELS unit)
add_executable(model_bank_parse_tests ${CMAKE_SOURCE_DIR}/tests/render/model_bank_parse_tests.c)
target_link_libraries(model_bank_parse_tests PRIVATE rage-data)
target_compile_options(model_bank_parse_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME model_bank_parse COMMAND model_bank_parse_tests)
set_tests_properties(model_bank_parse PROPERTIES LABELS unit)
add_executable(car_model_data_tests ${CMAKE_SOURCE_DIR}/tests/render/car_model_data_tests.c)
target_sources(car_model_data_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/native_mesh_writer.c
    ${CMAKE_SOURCE_DIR}/src/port/native_car_cache.c)
target_include_directories(car_model_data_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port)
target_link_libraries(car_model_data_tests PRIVATE rage-data rage-rmesh)
target_compile_options(car_model_data_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME car_model_data COMMAND car_model_data_tests)
set_tests_properties(car_model_data PROPERTIES LABELS unit)
add_executable(native_texture_tests ${CMAKE_SOURCE_DIR}/tests/render/native_texture_tests.c)
target_sources(native_texture_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port/native_texture.c)
target_include_directories(native_texture_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)
target_include_directories(native_texture_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port)
target_compile_options(native_texture_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(native_texture_tests PRIVATE rage-data)
add_test(NAME native_texture COMMAND native_texture_tests)
set_tests_properties(native_texture PROPERTIES LABELS unit)
# Mesh construction owns its output and needs neither live slots nor a GPU.
add_executable(native_mesh_writer_tests
    ${CMAKE_SOURCE_DIR}/tests/port/native_mesh_writer_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/native_mesh_writer.c)
target_include_directories(native_mesh_writer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port ${CMAKE_SOURCE_DIR}/include)
target_compile_options(native_mesh_writer_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(native_mesh_writer_tests PRIVATE rage-rmesh rage-sim)
add_test(NAME native_mesh_writer COMMAND native_mesh_writer_tests)
set_tests_properties(native_mesh_writer PROPERTIES LABELS unit)

# Renderer CPU state/interpolation tests need no window or game state.
add_executable(render_world_tests ${CMAKE_SOURCE_DIR}/tests/render/render_world_tests.c)
target_compile_options(render_world_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(render_world_tests PRIVATE rage-render-world rage-sim)
add_test(NAME render_world COMMAND render_world_tests)
set_tests_properties(render_world PROPERTIES LABELS unit)

# Image validation has no upload, game-state or platform dependency.
add_executable(image_asset_tests ${CMAKE_SOURCE_DIR}/tests/render/image_asset_tests.c)
target_link_libraries(image_asset_tests PRIVATE rage-data)
target_compile_options(image_asset_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME image_asset COMMAND image_asset_tests)
set_tests_properties(image_asset PROPERTIES LABELS unit)

add_executable(sim_hud_tests ${CMAKE_SOURCE_DIR}/tests/render/sim_hud_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/race_hud.c)
target_include_directories(sim_hud_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/port)
target_compile_options(sim_hud_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(sim_hud_tests PRIVATE rage-sim)
add_test(NAME sim_hud COMMAND sim_hud_tests)
set_tests_properties(sim_hud PROPERTIES LABELS unit)

add_executable(race_grid_tests ${CMAKE_SOURCE_DIR}/tests/car/race_grid_tests.c)
target_include_directories(race_grid_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(race_grid_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(race_grid_tests PRIVATE rage-data)
add_test(NAME race_grid COMMAND race_grid_tests)
set_tests_properties(race_grid PROPERTIES LABELS unit)

# Device sampling and its legacy adapter use no platform library.
add_executable(player_input_tests ${CMAKE_SOURCE_DIR}/tests/car/player_input_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_input.c)
target_include_directories(player_input_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(player_input_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(player_input_tests PRIVATE rage-sim)
add_test(NAME player_input COMMAND player_input_tests)
set_tests_properties(player_input PROPERTIES LABELS unit)

add_executable(scene_blocks_tests ${CMAKE_SOURCE_DIR}/tests/car/scene_blocks_tests.c)
target_include_directories(scene_blocks_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(scene_blocks_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(scene_blocks_tests PRIVATE rage-sim)
add_test(NAME scene_blocks COMMAND scene_blocks_tests)
set_tests_properties(scene_blocks PROPERTIES LABELS unit)

add_executable(terrain_bank_tests ${CMAKE_SOURCE_DIR}/tests/render/terrain_bank_tests.c)
target_link_libraries(terrain_bank_tests PRIVATE rage-data)
target_compile_options(terrain_bank_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME terrain_bank COMMAND terrain_bank_tests)
set_tests_properties(terrain_bank PROPERTIES LABELS unit)

add_executable(native_terrain_material_tests ${CMAKE_SOURCE_DIR}/tests/port/native_terrain_material_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/native_mesh_writer.c)
target_include_directories(native_terrain_material_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port)
target_link_libraries(native_terrain_material_tests PRIVATE rage-data rage-rmesh)
target_compile_options(native_terrain_material_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME native_terrain_material COMMAND native_terrain_material_tests)
set_tests_properties(native_terrain_material PROPERTIES LABELS unit)

add_executable(environment_data_tests ${CMAKE_SOURCE_DIR}/tests/render/environment_data_tests.c)
target_link_libraries(environment_data_tests PRIVATE rage-data)
target_compile_options(environment_data_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME environment_data COMMAND environment_data_tests)
set_tests_properties(environment_data PROPERTIES LABELS unit)

add_executable(environment_state_tests ${CMAKE_SOURCE_DIR}/tests/render/environment_state_tests.c)
target_link_libraries(environment_state_tests PRIVATE rage-data)
target_compile_options(environment_state_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME environment_state COMMAND environment_state_tests)
set_tests_properties(environment_state PROPERTIES LABELS unit)

add_executable(track_look_tests ${CMAKE_SOURCE_DIR}/tests/render/track_look_tests.c)
target_link_libraries(track_look_tests PRIVATE rage-data)
target_compile_options(track_look_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME track_look COMMAND track_look_tests)
set_tests_properties(track_look PROPERTIES LABELS unit)

add_executable(environment_view_tests ${CMAKE_SOURCE_DIR}/tests/render/environment_view_tests.c)
target_include_directories(environment_view_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/port)
target_compile_options(environment_view_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(environment_view_tests PRIVATE rage-data)
add_test(NAME environment_view COMMAND environment_view_tests)
set_tests_properties(environment_view PROPERTIES LABELS unit)

add_executable(native_sky_tests ${CMAKE_SOURCE_DIR}/tests/render/native_sky_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/native_sky.c ${CMAKE_SOURCE_DIR}/src/port/native_texture.c
    ${CMAKE_SOURCE_DIR}/src/port/sky_panorama_layout.c)
target_include_directories(native_sky_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/port)
target_link_libraries(native_sky_tests PRIVATE rage-data)
target_compile_options(native_sky_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME native_sky COMMAND native_sky_tests)
set_tests_properties(native_sky PROPERTIES LABELS unit)

add_executable(client_assets_tests ${CMAKE_SOURCE_DIR}/tests/render/client_assets_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/client_assets.c ${CMAKE_SOURCE_DIR}/src/port/native_texture.c)
target_include_directories(client_assets_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/port)
target_link_libraries(client_assets_tests PRIVATE rage-data rage-rmesh)
target_compile_options(client_assets_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME client_assets COMMAND client_assets_tests)
set_tests_properties(client_assets PROPERTIES LABELS unit)

add_executable(modern_prepared_meshes_tests ${CMAKE_SOURCE_DIR}/tests/render/modern_prepared_meshes_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/modern/modern_prepared_meshes.c)
target_include_directories(modern_prepared_meshes_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/port/modern)
target_link_libraries(modern_prepared_meshes_tests PRIVATE rage-render-world)
target_compile_options(modern_prepared_meshes_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
if(MSVC)
    target_compile_options(modern_prepared_meshes_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(modern_prepared_meshes_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME modern_prepared_meshes COMMAND modern_prepared_meshes_tests)
set_tests_properties(modern_prepared_meshes PROPERTIES LABELS unit)
