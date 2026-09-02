add_executable(draw_area_packet_tests
    draw_area_packet_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/draw_area_packet.c)
target_include_directories(draw_area_packet_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(draw_area_packet_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(draw_area_packet_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(draw_area_packet_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME draw_area_packet COMMAND draw_area_packet_tests)
set_tests_properties(draw_area_packet PROPERTIES LABELS unit)

# Where the race HUD's sprites are built, as opposed to what the drawing
# repositions each frame.
add_executable(race_hud_layout_tests
    race_hud_layout_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/build_race_hud_prims.c
    ${CMAKE_SOURCE_DIR}/src/port/hud_config.c)
target_include_directories(race_hud_layout_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(race_hud_layout_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(race_hud_layout_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include")
endif()
target_link_libraries(race_hud_layout_tests PRIVATE psyz)
add_test(NAME race_hud_layout COMMAND race_hud_layout_tests)
set_tests_properties(race_hud_layout PROPERTIES LABELS functional)

add_executable(race_hud_placement_tests
    race_hud_placement_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/build_race_hud_prims.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/build_sprite_from_desc.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/race_hud.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/draw_split_times.c
    ${CMAKE_SOURCE_DIR}/src/port/hud_config.c)
target_include_directories(race_hud_placement_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(race_hud_placement_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include")
endif()
target_link_libraries(race_hud_placement_tests PRIVATE psyz)
add_test(NAME race_hud_placement COMMAND race_hud_placement_tests)
set_tests_properties(race_hud_placement PROPERTIES LABELS functional)

add_executable(hud_config_tests
    hud_config_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/hud_config.c)
target_include_directories(hud_config_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hud_config_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME hud_config COMMAND hud_config_tests)

set_tests_properties(hud_config PROPERTIES LABELS functional)
# What the renderer draws for one asset, turned all the way round. Needs a
# GPU and the imported asset cache, and skips when either is absent; it never
# needs the game or the disc.
if(TARGET rage-render-stage)
    add_test(
        NAME render_stage_angles
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_render_stage_angles.py
            $<TARGET_FILE:rage-render-stage>
            ${CMAKE_SOURCE_DIR})
    set_tests_properties(render_stage_angles PROPERTIES
        LABELS functional
        TIMEOUT 600
        SKIP_RETURN_CODE 77)
endif()

# Where a stage puts its camera: pure scene composition, no GPU involved.
add_executable(render_stage_tests render_stage_tests.c)
target_link_libraries(render_stage_tests PRIVATE rage-render-world)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_stage_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME render_stage COMMAND render_stage_tests)
set_tests_properties(render_stage PROPERTIES LABELS functional)
add_executable(rmesh_tests rmesh_tests.c)
target_link_libraries(rmesh_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rmesh_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME rmesh COMMAND rmesh_tests)

set_tests_properties(rmesh PROPERTIES LABELS functional)
add_executable(rmesh_index_tests rmesh_index_tests.c)
target_link_libraries(rmesh_index_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rmesh_index_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME rmesh_index COMMAND rmesh_index_tests)

set_tests_properties(rmesh_index PROPERTIES LABELS functional)
add_executable(asset_id_tests asset_id_tests.c)
target_link_libraries(asset_id_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(asset_id_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME asset_id COMMAND asset_id_tests)

set_tests_properties(asset_id PROPERTIES LABELS functional)

add_executable(model_banks_tests
    model_banks_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/asset/model_banks.c)
target_include_directories(model_banks_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(model_banks_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(model_banks_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(model_banks_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME model_banks COMMAND model_banks_tests)
set_tests_properties(model_banks PROPERTIES LABELS unit)
add_executable(car_paint_tests car_paint_tests.c)
target_link_libraries(car_paint_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_paint_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME car_paint COMMAND car_paint_tests)

set_tests_properties(car_paint PROPERTIES LABELS functional)
add_executable(texture_mipmap_tests texture_mipmap_tests.c)
target_link_libraries(texture_mipmap_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(texture_mipmap_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME texture_mipmap COMMAND texture_mipmap_tests)

set_tests_properties(texture_mipmap PROPERTIES LABELS functional)
add_executable(render_material_tests render_material_tests.c)
target_link_libraries(render_material_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_material_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME render_material COMMAND render_material_tests)

set_tests_properties(render_material PROPERTIES LABELS functional)
add_executable(rmesh_cache_tests rmesh_cache_tests.c)
target_link_libraries(rmesh_cache_tests PRIVATE rage-rmesh)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rmesh_cache_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME rmesh_cache COMMAND rmesh_cache_tests)

set_tests_properties(rmesh_cache PROPERTIES LABELS functional)
add_executable(visible_cell_tests
    visible_cell_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/visible_cell_lookup.c)
target_include_directories(visible_cell_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(visible_cell_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include")
endif()
target_link_libraries(visible_cell_tests PRIVATE psyz)
add_test(NAME visible_cell COMMAND visible_cell_tests)
set_tests_properties(visible_cell PROPERTIES LABELS functional)

add_executable(visible_cell_scan_tests
    visible_cell_scan_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/visible_cell_scan.c)
target_include_directories(visible_cell_scan_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(visible_cell_scan_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
add_test(NAME visible_cell_scan COMMAND visible_cell_scan_tests)
set_tests_properties(visible_cell_scan PROPERTIES LABELS unit)

add_executable(route_scenery_tests
    route_scenery_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_route_scenery.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c)
target_include_directories(route_scenery_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(route_scenery_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(route_scenery_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(route_scenery_tests PRIVATE psyz)
add_test(NAME route_scenery COMMAND route_scenery_tests)
set_tests_properties(route_scenery PROPERTIES LABELS functional)

add_executable(path_scenery_init_tests
    path_scenery_init_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/init_path_scenery.c)
target_include_directories(path_scenery_init_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(path_scenery_init_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(path_scenery_init_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(path_scenery_init_tests PRIVATE psyz)
add_test(NAME path_scenery_init COMMAND path_scenery_init_tests)
set_tests_properties(path_scenery_init PROPERTIES LABELS functional)

add_executable(path_scenery_playback_tests
    path_scenery_playback_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/init_path_scenery.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_path_scenery.c)
target_include_directories(path_scenery_playback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(path_scenery_playback_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(path_scenery_playback_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(path_scenery_playback_tests PRIVATE psyz)
add_test(NAME path_scenery_playback COMMAND path_scenery_playback_tests)
set_tests_properties(path_scenery_playback PROPERTIES LABELS functional)

add_executable(flyby_scenery_seed_tests
    flyby_scenery_seed_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/seed_flyby_scenery.c)
target_include_directories(flyby_scenery_seed_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(flyby_scenery_seed_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(flyby_scenery_seed_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(flyby_scenery_seed_tests PRIVATE psyz)
add_test(NAME flyby_scenery_seed COMMAND flyby_scenery_seed_tests)
set_tests_properties(flyby_scenery_seed PROPERTIES LABELS functional)

add_executable(flyby_scenery_playback_tests
    flyby_scenery_playback_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/flyby_scenery.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c)
target_include_directories(flyby_scenery_playback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(flyby_scenery_playback_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(flyby_scenery_playback_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(flyby_scenery_playback_tests PRIVATE psyz)
add_test(NAME flyby_scenery_playback COMMAND flyby_scenery_playback_tests)
set_tests_properties(flyby_scenery_playback PROPERTIES LABELS functional)

add_executable(course_scenery_dispatch_tests
    course_scenery_dispatch_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/course_scenery.c)
target_include_directories(course_scenery_dispatch_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(course_scenery_dispatch_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(course_scenery_dispatch_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(course_scenery_dispatch_tests PRIVATE psyz)
add_test(NAME course_scenery_dispatch COMMAND course_scenery_dispatch_tests)
set_tests_properties(course_scenery_dispatch PROPERTIES LABELS functional)

add_executable(shuttle_scenery_init_tests
    shuttle_scenery_init_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/init_shuttle_scenery.c)
target_include_directories(shuttle_scenery_init_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(shuttle_scenery_init_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(shuttle_scenery_init_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(shuttle_scenery_init_tests PRIVATE psyz)
add_test(NAME shuttle_scenery_init COMMAND shuttle_scenery_init_tests)
set_tests_properties(shuttle_scenery_init PROPERTIES LABELS functional)

add_executable(shuttle_scenery_playback_tests
    shuttle_scenery_playback_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_shuttle_scenery.c)
target_include_directories(shuttle_scenery_playback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(shuttle_scenery_playback_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(shuttle_scenery_playback_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(shuttle_scenery_playback_tests PRIVATE psyz)
add_test(NAME shuttle_scenery_playback COMMAND shuttle_scenery_playback_tests)
set_tests_properties(shuttle_scenery_playback PROPERTIES LABELS functional)

add_executable(shuttle_scenery_draw_tests
    shuttle_scenery_draw_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/draw_shuttle_scenery.c)
target_include_directories(shuttle_scenery_draw_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(shuttle_scenery_draw_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(shuttle_scenery_draw_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(shuttle_scenery_draw_tests PRIVATE psyz)
add_test(NAME shuttle_scenery_draw COMMAND shuttle_scenery_draw_tests)
set_tests_properties(shuttle_scenery_draw PROPERTIES LABELS functional)

add_executable(spinning_scenery_tests
    spinning_scenery_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/draw_spinning_scenery.c)
target_include_directories(spinning_scenery_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(spinning_scenery_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(spinning_scenery_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(spinning_scenery_tests PRIVATE psyz)
add_test(NAME spinning_scenery COMMAND spinning_scenery_tests)
set_tests_properties(spinning_scenery PROPERTIES LABELS functional)

add_executable(static_scenery_tests
    static_scenery_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/draw_static_scenery.c)
target_include_directories(static_scenery_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(static_scenery_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(static_scenery_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(static_scenery_tests PRIVATE psyz)
add_test(NAME static_scenery COMMAND static_scenery_tests)
set_tests_properties(static_scenery PROPERTIES LABELS functional)

add_executable(scripted_scenery_dispatch_tests
    scripted_scenery_dispatch_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/draw_scripted_scenery.c)
target_include_directories(scripted_scenery_dispatch_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(scripted_scenery_dispatch_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(scripted_scenery_dispatch_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(scripted_scenery_dispatch_tests PRIVATE psyz)
add_test(NAME scripted_scenery_dispatch COMMAND scripted_scenery_dispatch_tests)
set_tests_properties(scripted_scenery_dispatch PROPERTIES LABELS functional)

add_executable(animated_scenery_tests
    animated_scenery_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/animated_scenery.c)
target_include_directories(animated_scenery_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(animated_scenery_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(animated_scenery_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(animated_scenery_tests PRIVATE psyz)
add_test(NAME animated_scenery COMMAND animated_scenery_tests)
set_tests_properties(animated_scenery PROPERTIES LABELS functional)

add_executable(start_grid_scenery_tests
    start_grid_scenery_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/draw_start_grid_scenery.c)
target_include_directories(start_grid_scenery_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(start_grid_scenery_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(start_grid_scenery_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(start_grid_scenery_tests PRIVATE psyz)
add_test(NAME start_grid_scenery COMMAND start_grid_scenery_tests)
set_tests_properties(start_grid_scenery PROPERTIES LABELS functional)

add_executable(track_event_sound_tests
    track_event_sound_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_track_event_sound.c)
target_include_directories(track_event_sound_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(track_event_sound_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_event_sound_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(track_event_sound_tests PRIVATE psyz)
add_test(NAME track_event_sound COMMAND track_event_sound_tests)
set_tests_properties(track_event_sound PROPERTIES LABELS functional)

add_executable(point_ambience_tests
    point_ambience_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_point_ambience.c)
target_include_directories(point_ambience_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(point_ambience_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(point_ambience_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(point_ambience_tests PRIVATE psyz)
add_test(NAME point_ambience COMMAND point_ambience_tests)
set_tests_properties(point_ambience PROPERTIES LABELS functional)

add_executable(zone_ambience_tests
    zone_ambience_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_zone_ambience.c)
target_include_directories(zone_ambience_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(zone_ambience_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(zone_ambience_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(zone_ambience_tests PRIVATE psyz)
add_test(NAME zone_ambience COMMAND zone_ambience_tests)
set_tests_properties(zone_ambience PROPERTIES LABELS functional)

add_executable(race_cue_tests
    race_cue_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/trigger_race_cues.c)
target_include_directories(race_cue_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(race_cue_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(race_cue_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(race_cue_tests PRIVATE psyz)
add_test(NAME race_cue COMMAND race_cue_tests)
set_tests_properties(race_cue PROPERTIES LABELS functional)

add_executable(environment_script_tests
    environment_script_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/environment_script.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/load_environment_cue.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/env_palette.c)
target_include_directories(environment_script_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(environment_script_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(environment_script_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(environment_script_tests PRIVATE psyz)
add_test(NAME environment_script COMMAND environment_script_tests)
set_tests_properties(environment_script PROPERTIES LABELS functional)

add_executable(track_zone_blend_tests
    track_zone_blend_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/get_track_zone_blend.c)
target_include_directories(track_zone_blend_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(track_zone_blend_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_zone_blend_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(track_zone_blend_tests PRIVATE psyz)
add_test(NAME track_zone_blend COMMAND track_zone_blend_tests)
set_tests_properties(track_zone_blend PROPERTIES LABELS functional)

add_executable(install_track_points_tests
    install_track_points_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/install_track_points.c)
target_include_directories(install_track_points_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(install_track_points_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(install_track_points_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(install_track_points_tests PRIVATE psyz)
add_test(NAME install_track_points COMMAND install_track_points_tests)
set_tests_properties(install_track_points PROPERTIES LABELS functional)

add_executable(mirror_pass_tests
    mirror_pass_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/mirror_pass.c)
target_include_directories(mirror_pass_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(mirror_pass_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mirror_pass_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(mirror_pass_tests PRIVATE psyz)
add_test(NAME mirror_pass COMMAND mirror_pass_tests)
set_tests_properties(mirror_pass PROPERTIES LABELS functional)

add_test(NAME hud_anchoring
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/verify_hud_anchoring.py)
set_tests_properties(hud_anchoring PROPERTIES LABELS functional)

add_test(NAME native_sky_projection
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_sky_projection.py)

set_tests_properties(native_sky_projection PROPERTIES LABELS functional)
add_executable(render_world_tests render_world_tests.c)
target_link_libraries(render_world_tests PRIVATE rage-render-world)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_world_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME render_world COMMAND render_world_tests)

set_tests_properties(render_world PROPERTIES LABELS functional)
add_executable(render_world_snapshot_tests render_world_snapshot_tests.c)
target_link_libraries(render_world_snapshot_tests PRIVATE rage-render-world)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_world_snapshot_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME render_world_snapshot COMMAND render_world_snapshot_tests)

set_tests_properties(render_world_snapshot PROPERTIES LABELS functional)
add_executable(track_asset_identity_tests
    track_asset_identity_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/track_asset_identity.c)
target_include_directories(track_asset_identity_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_asset_identity_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME track_asset_identity COMMAND track_asset_identity_tests)

set_tests_properties(track_asset_identity PROPERTIES LABELS functional)
add_executable(track_lighting_tests
    track_lighting_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/track_lighting.c)
target_include_directories(track_lighting_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_lighting_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME track_lighting COMMAND track_lighting_tests)

set_tests_properties(track_lighting PROPERTIES LABELS functional)
add_executable(zone_lighting_tests
    zone_lighting_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/track_lighting.c)
target_include_directories(zone_lighting_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(zone_lighting_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(zone_lighting_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(zone_lighting_tests PRIVATE psyz)
add_test(NAME zone_lighting COMMAND zone_lighting_tests)
set_tests_properties(zone_lighting PROPERTIES LABELS unit)

add_executable(course_coordinate_tests course_coordinate_tests.c)
target_include_directories(course_coordinate_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(course_coordinate_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME course_coordinate COMMAND course_coordinate_tests)

set_tests_properties(course_coordinate PROPERTIES LABELS functional)
add_executable(render_mesh_build_tests render_mesh_build_tests.c)
target_link_libraries(render_mesh_build_tests PRIVATE rage-render-world)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(render_mesh_build_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME render_mesh_build COMMAND render_mesh_build_tests)

set_tests_properties(render_mesh_build PROPERTIES LABELS functional)
add_executable(native_geometry_interpolation_tests
    native_geometry_interpolation_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/native_geometry_interpolation.c)
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

set_tests_properties(native_geometry_interpolation PROPERTIES LABELS functional)

if(RAGE_RUBY_EXECUTABLE)
add_test(
    NAME native_frame_offsets
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_frame_offsets.rb
        ${CMAKE_SOURCE_DIR}
)

set_tests_properties(native_frame_offsets PROPERTIES LABELS functional)
add_test(
    NAME frame_environment_aliases
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_frame_environment_aliases.rb
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/display_setup.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/mirror_pass.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/tachometer_needle.c
        ${RAGE_HOST_STATE_SOURCES}
)

set_tests_properties(frame_environment_aliases PROPERTIES LABELS functional)
add_test(
    NAME capture_sync
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_capture_sync.rb
        ${CMAKE_SOURCE_DIR}/src/port/main_smoke.c
)

set_tests_properties(capture_sync PROPERTIES LABELS functional)
endif()

add_executable(display_setup_tests
    display_setup_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/display_setup.c)
target_include_directories(display_setup_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(display_setup_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(display_setup_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(display_setup_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(display_setup_tests PRIVATE psyz)
add_test(NAME display_setup COMMAND display_setup_tests)
set_tests_properties(display_setup PROPERTIES LABELS unit)

add_executable(track_math_tests
    track_math_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/track_textures.c)
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
set_tests_properties(track_math PROPERTIES LABELS functional)

add_executable(draw_lap_number_tests
    draw_lap_number_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/draw_lap_number.c)
target_include_directories(draw_lap_number_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(draw_lap_number_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(draw_lap_number_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(draw_lap_number_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(draw_lap_number_tests PRIVATE -Wall -Wextra -Werror)
endif()
target_link_libraries(draw_lap_number_tests PRIVATE psyz)
add_test(NAME draw_lap_number COMMAND draw_lap_number_tests)
set_tests_properties(draw_lap_number PROPERTIES LABELS functional)

add_executable(draw_prims_tests
    draw_prims_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/draw_prims.c)
target_include_directories(draw_prims_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(draw_prims_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(draw_prims_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(draw_prims_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(draw_prims_tests PRIVATE -Wall -Wextra -Werror)
endif()
target_link_libraries(draw_prims_tests PRIVATE psyz)
add_test(NAME draw_prims COMMAND draw_prims_tests)
set_tests_properties(draw_prims PROPERTIES LABELS functional)

add_executable(draw_packet_queue_tests
    draw_packet_queue_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/draw_packet_queue.c)
target_include_directories(draw_packet_queue_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(draw_packet_queue_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(draw_packet_queue_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(draw_packet_queue_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(draw_packet_queue_tests PRIVATE psyz)
add_test(NAME draw_packet_queue COMMAND draw_packet_queue_tests)
set_tests_properties(draw_packet_queue PROPERTIES LABELS unit)

add_executable(object_matrix_tests
    object_matrix_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/object_matrix.c)
target_include_directories(object_matrix_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(object_matrix_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(object_matrix_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(object_matrix_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(object_matrix_tests PRIVATE psyz)
add_test(NAME object_matrix COMMAND object_matrix_tests)
set_tests_properties(object_matrix PROPERTIES LABELS unit)

add_executable(car_render_rules_tests
    car_render_rules_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/car_render_rules.c)
target_include_directories(car_render_rules_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_render_rules_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME car_render_rules COMMAND car_render_rules_tests)
set_tests_properties(car_render_rules PROPERTIES LABELS unit)

add_executable(car_model_matrix_tests
    car_model_matrix_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/car_model_matrix.c)
target_include_directories(car_model_matrix_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_model_matrix_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_model_matrix_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_model_matrix_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME car_model_matrix COMMAND car_model_matrix_tests)
set_tests_properties(car_model_matrix PROPERTIES LABELS unit)

add_executable(add_tile_prim_tests
    add_tile_prim_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/add_tile_prim.c)
target_include_directories(add_tile_prim_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(add_tile_prim_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(add_tile_prim_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(add_tile_prim_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(add_tile_prim_tests PRIVATE psyz)
add_test(NAME add_tile_prim COMMAND add_tile_prim_tests)
set_tests_properties(add_tile_prim PROPERTIES LABELS unit)

add_executable(basic_text_tests
    basic_text_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/text_8x8.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/sprite_string.c)
target_include_directories(basic_text_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(basic_text_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(basic_text_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(basic_text_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(basic_text_tests PRIVATE psyz)
add_test(NAME basic_text COMMAND basic_text_tests)
set_tests_properties(basic_text PROPERTIES LABELS unit)

add_executable(atlas_text_tests
    atlas_text_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/atlas_text.c)
target_include_directories(atlas_text_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(atlas_text_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(atlas_text_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(atlas_text_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME atlas_text COMMAND atlas_text_tests)
set_tests_properties(atlas_text PROPERTIES LABELS unit)

add_executable(bit_pattern_overlay_tests
    bit_pattern_overlay_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/bit_pattern_overlay.c)
target_include_directories(bit_pattern_overlay_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(bit_pattern_overlay_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(bit_pattern_overlay_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(bit_pattern_overlay_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME bit_pattern_overlay COMMAND bit_pattern_overlay_tests)
set_tests_properties(bit_pattern_overlay PROPERTIES LABELS unit)

add_executable(number_drawing_tests
    number_drawing_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/number_drawing.c)
target_include_directories(number_drawing_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(number_drawing_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(number_drawing_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(number_drawing_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME number_drawing COMMAND number_drawing_tests)
set_tests_properties(number_drawing PROPERTIES LABELS unit)

add_executable(hud_packet_emitters_tests
    hud_packet_emitters_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/draw_hud_digit.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/draw_wrong_way_warning.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/fullscreen_fade.c)
target_include_directories(hud_packet_emitters_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(hud_packet_emitters_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(hud_packet_emitters_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(hud_packet_emitters_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(hud_packet_emitters_tests PRIVATE psyz)
add_test(NAME hud_packet_emitters COMMAND hud_packet_emitters_tests)
set_tests_properties(hud_packet_emitters PROPERTIES LABELS unit)

add_executable(tachometer_tests
    tachometer_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/draw_tachometer.c)
target_include_directories(tachometer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(tachometer_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(tachometer_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(tachometer_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(tachometer_tests PRIVATE psyz)
add_test(NAME tachometer COMMAND tachometer_tests)
set_tests_properties(tachometer PROPERTIES LABELS functional)

add_executable(boot_logo_tests
    boot_logo_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/boot/draw_boot_logo.c)
target_include_directories(boot_logo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(boot_logo_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(boot_logo_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(boot_logo_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME boot_logo COMMAND boot_logo_tests)
set_tests_properties(boot_logo PROPERTIES LABELS unit)

add_executable(boot_logo_scene_tests
    boot_logo_scene_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/boot/boot_scene.c)
target_include_directories(boot_logo_scene_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(boot_logo_scene_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(boot_logo_scene_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(boot_logo_scene_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME boot_logo_scene COMMAND boot_logo_scene_tests)
set_tests_properties(boot_logo_scene PROPERTIES LABELS functional)

add_executable(init_subsystems_tests
    init_subsystems_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/boot/init_subsystems.c)
target_include_directories(init_subsystems_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(init_subsystems_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(init_subsystems_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(init_subsystems_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME init_subsystems COMMAND init_subsystems_tests)
set_tests_properties(init_subsystems PROPERTIES LABELS functional)

add_executable(proportional_text_tests
    proportional_text_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/text.c)
target_include_directories(proportional_text_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(proportional_text_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(proportional_text_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(proportional_text_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(proportional_text_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
target_link_libraries(proportional_text_tests PRIVATE psyz)
add_test(NAME proportional_text COMMAND proportional_text_tests)
set_tests_properties(proportional_text PROPERTIES LABELS unit)

if(TARGET rage-racer-smoke)
    add_test(
        NAME boot_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_boot_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(boot_frame PROPERTIES LABELS e2e)
    set_tests_properties(boot_frame PROPERTIES TIMEOUT 60)
    add_test(
        NAME title_page_stability
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_title_page_stability.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(title_page_stability PROPERTIES LABELS e2e)
    set_tests_properties(title_page_stability PROPERTIES TIMEOUT 60)
    add_test(
        NAME renderer_toggle_cycles
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_renderer_toggle_cycles.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(renderer_toggle_cycles PROPERTIES LABELS e2e)
    set_tests_properties(renderer_toggle_cycles PROPERTIES TIMEOUT 195)
    add_test(NAME mirrored_track COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirrored_track.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
set_tests_properties(mirrored_track PROPERTIES LABELS e2e)
    set_tests_properties(mirrored_track PROPERTIES TIMEOUT 360)
    add_test(
        NAME prologue_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_prologue_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(prologue_frame PROPERTIES LABELS e2e)
    set_tests_properties(prologue_frame PROPERTIES TIMEOUT 90)
    add_test(
        NAME japanese_prologue_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_prologue_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
            japanese
    )
set_tests_properties(japanese_prologue_frame PROPERTIES LABELS e2e)
    set_tests_properties(japanese_prologue_frame PROPERTIES TIMEOUT 90)

    add_test(
        NAME scene_capture
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_scene_capture.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(scene_capture PROPERTIES LABELS e2e)
    set_tests_properties(scene_capture PROPERTIES TIMEOUT 360)
    add_test(
        NAME modern_feature_frames
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_modern_feature_frames.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(modern_feature_frames PROPERTIES LABELS e2e)
    set_tests_properties(modern_feature_frames PROPERTIES TIMEOUT 360)

    add_test(
        NAME pegase_cabin
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_pegase_cabin.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(pegase_cabin PROPERTIES LABELS e2e)
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
set_tests_properties(sky_frames PROPERTIES LABELS e2e)
    set_tests_properties(sky_frames PROPERTIES TIMEOUT 600)

    add_test(
        NAME native_render_world
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_render_world.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(native_render_world PROPERTIES LABELS e2e)
    set_tests_properties(native_render_world PROPERTIES TIMEOUT 120)

    add_test(
        NAME grand_prix_terrain_frame
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_grand_prix_terrain_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(grand_prix_terrain_frame PROPERTIES LABELS e2e)
    set_tests_properties(grand_prix_terrain_frame PROPERTIES TIMEOUT 120)

    add_test(
        NAME mirror_entry
        COMMAND ruby
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirror_entry.rb
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(mirror_entry PROPERTIES LABELS e2e)
    set_tests_properties(mirror_entry PROPERTIES TIMEOUT 150)
    add_test(NAME mirror_cars COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_mirror_cars.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
set_tests_properties(mirror_cars PROPERTIES LABELS e2e)
    set_tests_properties(mirror_cars PROPERTIES TIMEOUT 360)

    add_test(
        NAME waterfall
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_waterfall.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(waterfall PROPERTIES LABELS e2e)
    set_tests_properties(waterfall PROPERTIES TIMEOUT 180)
endif()
