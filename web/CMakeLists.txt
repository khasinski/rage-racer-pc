# Rage Racer in the browser: a standalone project, separate from the desktop
# build. engine/ is this project's own copy of the desktop C it needs (race
# simulation, disc import, meshes, render world and client race), taken from
# the repository's src/, include/ and external/ with the same relative paths.
# Change it freely: the desktop build never sees it.
#
#   emcmake cmake -S web -B build/web -DCMAKE_BUILD_TYPE=Release        rage-web
#   cmake -S web -B build/web-parity -DCMAKE_BUILD_TYPE=Release         rage-web-parity
cmake_minimum_required(VERSION 3.20)
project(rage_web C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(_POSIX_C_SOURCE=200809L _XOPEN_SOURCE=700)
endif()
add_compile_definitions(RAGE_HAS_AUTHORED_CARS=0)

set(ENGINE ${CMAKE_CURRENT_SOURCE_DIR}/engine)
set(WARNINGS -Wall -Wextra -Werror)

# Retail race simulation (fixed-point physics, AI, track).
set(SIM_SOURCES
    src/main/PAL/main/asset/read_car_spec.c
    src/main/PAL/main/asset/read_scene_blocks.c
    src/main/PAL/main/car/angle_math.c
    src/main/PAL/main/car/arc_measure.c
    src/main/PAL/main/car/begin_car_standing_start.c
    src/main/PAL/main/car/car_airborne.c
    src/main/PAL/main/car/car_body_kick.c
    src/main/PAL/main/car/car_body_roll.c
    src/main/PAL/main/car/car_collision_response.c
    src/main/PAL/main/car/car_contact.c
    src/main/PAL/main/car/car_crest_motion.c
    src/main/PAL/main/car/car_dynamics.c
    src/main/PAL/main/car/car_facing_direction.c
    src/main/PAL/main/car/car_hulls.c
    src/main/PAL/main/car/car_jump_arc.c
    src/main/PAL/main/car/car_knockback.c
    src/main/PAL/main/car/car_launch.c
    src/main/PAL/main/car/car_performance_setup.c
    src/main/PAL/main/car/car_shift_pitch.c
    src/main/PAL/main/car/car_slide.c
    src/main/PAL/main/car/car_standing_start.c
    src/main/PAL/main/car/car_track_math.c
    src/main/PAL/main/car/car_track_state.c
    src/main/PAL/main/car/car_track_steering.c
    src/main/PAL/main/car/car_wheel_rotation.c
    src/main/PAL/main/car/catalog_override.c
    src/main/PAL/main/car/catalog_parse.c
    src/main/PAL/main/car/clamp_car_lateral_offset.c
    src/main/PAL/main/car/collision_quads.c
    src/main/PAL/main/car/driver.c
    src/main/PAL/main/car/driver_collision.c
    src/main/PAL/main/car/driver_field.c
    src/main/PAL/main/car/driver_init.c
    src/main/PAL/main/car/driver_input.c
    src/main/PAL/main/car/driver_input_valid.c
    src/main/PAL/main/car/drivetrain_loads.c
    src/main/PAL/main/car/drivetrain_shift.c
    src/main/PAL/main/car/drivetrain_torque.c
    src/main/PAL/main/car/engine_sound.c
    src/main/PAL/main/car/find_track_segment.c
    src/main/PAL/main/car/gear_shift.c
    src/main/PAL/main/car/integrate_player_position.c
    src/main/PAL/main/car/lap_progress.c
    src/main/PAL/main/car/player_body_offset.c
    src/main/PAL/main/car/player_contact_response.c
    src/main/PAL/main/car/player_controls.c
    src/main/PAL/main/car/player_jump.c
    src/main/PAL/main/car/player_tilt.c
    src/main/PAL/main/car/player_track_contact.c
    src/main/PAL/main/car/player_track_limits.c
    src/main/PAL/main/car/prepare_airborne_drivetrain.c
    src/main/PAL/main/car/replay_track_reconstruction.c
    src/main/PAL/main/car/rival_acceleration.c
    src/main/PAL/main/car/rival_ai_setup.c
    src/main/PAL/main/car/rival_body_motion.c
    src/main/PAL/main/car/rival_car_init.c
    src/main/PAL/main/car/rival_car_movement.c
    src/main/PAL/main/car/rival_collision.c
    src/main/PAL/main/car/rival_racing_line.c
    src/main/PAL/main/car/rival_step.c
    src/main/PAL/main/car/rival_target_speed.c
    src/main/PAL/main/car/rival_track_pose.c
    src/main/PAL/main/car/rival_traffic_avoidance.c
    src/main/PAL/main/car/seed_car_ai_speed_keys.c
    src/main/PAL/main/car/steer_car_along_route.c
    src/main/PAL/main/car/step_car_drivetrain.c
    src/main/PAL/main/car/update_car_body_kick.c
    src/main/PAL/main/car/update_car_driving.c
    src/main/PAL/main/car/update_car_travel_velocity.c
    src/main/PAL/main/math/angle.c
    src/main/PAL/main/math/cos_angle.c
    src/main/PAL/main/math/geometry.c
    src/main/PAL/main/math/hull_rotation.c
    src/main/PAL/main/math/sin_angle.c
    src/main/PAL/main/race/race_frame.c
    src/main/PAL/main/race/race_lap.c
    src/main/PAL/main/race/race_sim.c
    src/main/PAL/main/race/time_conversion.c
    src/main/PAL/main/track/blend_angle.c
    src/main/PAL/main/track/interpolate_track_angle.c
    src/main/PAL/main/track/interpolate_track_point.c
    src/main/PAL/main/track/read_track_events.c
    src/main/PAL/main/track/read_track_route.c
    src/main/PAL/main/track/smooth_track_angle.c
    src/main/PAL/main/track/track_data.c
    src/main/PAL/main/track/track_zone.c)

# Disc image reading and retail race data import.
set(DATA_SOURCES
    src/main/PAL/main/asset/race_data.c
    src/main/PAL/main/asset/race_disc.c
    src/main/PAL/main/asset/race_grid.c
    src/main/PAL/main/asset/read_car_model.c
    src/main/PAL/main/asset/read_course_objects.c
    src/main/PAL/main/asset/read_image_asset.c
    src/main/PAL/main/asset/read_model_bank.c
    src/main/PAL/main/asset/read_terrain_bank.c
    src/main/PAL/main/asset/read_track_images.c
    src/main/PAL/main/asset/read_track_look.c
    src/main/PAL/main/track/environment.c
    src/main/PAL/main/track/read_environment_script.c
    src/main/PAL/main/track/scenery_data.c
    src/main/PAL/main/track/shuttle.c
    src/main/PAL/main/track/spinners.c
    src/port/archive_index.c
    src/port/disc_cue.c
    src/port/disc_discovery.c
    src/port/disc_identity.c
    src/port/disc_iso.c
    src/port/disc_raw_file.c)

# Runtime meshes, materials and texture mip chains.
set(RMESH_SOURCES
    src/render/asset_id.c
    src/render/asset_path.c
    src/render/authored_car_surface.c
    src/render/car_paint.c
    src/render/mod_file_policy.c
    src/render/mod_file_snapshot.c
    src/render/mod_manifest.c
    src/render/mod_provider.c
    src/render/mod_selection.c
    src/render/render_material.c
    src/render/rmesh.c
    src/render/rmesh_cache.c
    src/render/rmesh_index.c
    src/render/rmesh_replace.c
    src/render/texture_mipmap.c)

# Render world: instances, fog, lighting, shadow map and draw building.
set(RENDER_WORLD_SOURCES
    src/render/car_lamps.c
    src/render/car_lights.c
    src/render/render_geometry_pack.c
    src/render/render_mesh_build.c
    src/render/render_projection.c
    src/render/render_shadow.c
    src/render/render_stage.c
    src/render/render_world.c
    src/render/render_world_frame.c
    src/render/render_world_snapshot.c
    src/render/track_lighting.c)

# Client race: the scene around the simulation, as the multiplayer client.
set(CLIENT_SOURCES
    src/port/axis_curve.c
    src/port/car_parts.c
    src/port/chase_camera.c
    src/port/client_assets.c
    src/port/client_race.c
    src/port/client_world.c
    src/port/native_car_cache.c
    src/port/native_mesh_writer.c
    src/port/native_sky.c
    src/port/native_texture.c
    src/port/native_visibility.c
    src/port/race_view.c
    src/port/sky_panorama_layout.c)

foreach(list SIM_SOURCES DATA_SOURCES RMESH_SOURCES RENDER_WORLD_SOURCES CLIENT_SOURCES)
    list(TRANSFORM ${list} PREPEND ${ENGINE}/)
endforeach()

add_library(web-sim STATIC ${SIM_SOURCES})
target_include_directories(web-sim PRIVATE ${ENGINE}/include ${ENGINE}/external/psyz/psyz/src/psyz)
target_compile_options(web-sim PRIVATE -fno-strict-aliasing -fwrapv ${WARNINGS})

add_library(web-data STATIC ${DATA_SOURCES})
target_include_directories(web-data PUBLIC ${ENGINE}/include PRIVATE ${ENGINE}/src/port)
target_compile_options(web-data PRIVATE ${WARNINGS})
target_link_libraries(web-data PUBLIC web-sim)

add_library(web-rmesh STATIC ${RMESH_SOURCES})
target_include_directories(web-rmesh PUBLIC ${ENGINE}/src)
target_compile_options(web-rmesh PRIVATE ${WARNINGS})

add_library(web-render-world STATIC ${RENDER_WORLD_SOURCES})
target_include_directories(web-render-world PUBLIC ${ENGINE}/src ${ENGINE}/src/port/include)
target_compile_options(web-render-world PRIVATE ${WARNINGS})
target_link_libraries(web-render-world PUBLIC web-rmesh m)

set(BRIDGE_INCLUDES
    ${ENGINE}/src/render
    ${ENGINE}/include
    ${ENGINE}/src/port
    ${ENGINE}/src/port/include
    ${ENGINE}/src)
set(BRIDGE_LIBRARIES web-data web-render-world)
# Race rules shared by the browser module and the race server module.
set(RULES_SOURCES wasm/web_rules.c wasm/web_rules_exports.c)

if(EMSCRIPTEN)
    add_executable(rage-web wasm/rage_web.c wasm/web_hud.c wasm/web_sky.c ${RULES_SOURCES} ${CLIENT_SOURCES})
    target_include_directories(rage-web PRIVATE ${BRIDGE_INCLUDES})
    target_link_libraries(rage-web PRIVATE ${BRIDGE_LIBRARIES})
    set_target_properties(rage-web PROPERTIES SUFFIX ".mjs")
    target_link_options(rage-web PRIVATE
        -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createRageModule
        -sENVIRONMENT=web,worker,node
        -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=268435456 -sMAXIMUM_MEMORY=4294967296
        -sFORCE_FILESYSTEM=1
        -sEXPORTED_FUNCTIONS=_malloc,_free
        -sEXPORTED_RUNTIME_METHODS=FS,ccall,cwrap,HEAPU8,HEAPF32,HEAPU32,HEAP32,UTF8ToString)

    # Authoritative simulation for the multiplayer server (web/server), run by
    # Node; the disc is read in place through NODEFS.
    add_executable(rage-server wasm/rage_server.c ${RULES_SOURCES})
    target_include_directories(rage-server PRIVATE ${BRIDGE_INCLUDES})
    target_link_libraries(rage-server PRIVATE web-data web-sim)
    set_target_properties(rage-server PROPERTIES SUFFIX ".mjs")
    target_link_options(rage-server PRIVATE
        -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createRaceServerModule
        -sENVIRONMENT=node
        -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -sMAXIMUM_MEMORY=4294967296
        -sFORCE_FILESYSTEM=1 -lnodefs.js
        -sEXPORTED_FUNCTIONS=_malloc,_free
        -sEXPORTED_RUNTIME_METHODS=FS,NODEFS,ccall,HEAPU8,HEAP32,UTF8ToString)
else()
    # Native half of the physics parity check: the same bridge and engine
    # built by the host compiler, compared tick by tick with the WebAssembly.
    add_executable(rage-web-parity wasm/parity_main.c wasm/rage_web.c wasm/web_hud.c wasm/web_sky.c ${RULES_SOURCES} ${CLIENT_SOURCES})
    target_include_directories(rage-web-parity PRIVATE wasm/native_stub ${BRIDGE_INCLUDES})
    target_link_libraries(rage-web-parity PRIVATE ${BRIDGE_LIBRARIES})
endif()
