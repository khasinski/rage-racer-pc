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
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/frontend.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rear_view_mirror.c
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
