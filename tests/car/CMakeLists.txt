add_test(NAME native_car_names COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_car_names.py
    ${CMAKE_SOURCE_DIR})

set_tests_properties(native_car_names PROPERTIES LABELS functional)
add_executable(chase_camera_tests
    chase_camera_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/chase_camera.c)
target_include_directories(chase_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(chase_camera_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME chase_camera COMMAND chase_camera_tests)

set_tests_properties(chase_camera PROPERTIES LABELS functional)
add_test(
    NAME player_car_interior_aliases
    COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_player_car_aliases.py
        ${CMAKE_SOURCE_DIR}
)

set_tests_properties(player_car_interior_aliases PROPERTIES LABELS functional)

if(TARGET rage-host-state)
add_executable(select_track_camera_table_tests
    select_track_camera_table_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/select_track_camera_table.c)
target_include_directories(select_track_camera_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(select_track_camera_table_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(select_track_camera_table_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(select_track_camera_table_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME select_track_camera_table COMMAND select_track_camera_table_tests)
set_tests_properties(select_track_camera_table PROPERTIES LABELS unit)

add_executable(track_camera_tests
    track_camera_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/camera_chase.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/camera_common.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/track_camera_modes.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_camera.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rot_matrix.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c
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
set_tests_properties(track_camera PROPERTIES LABELS functional)

add_executable(finish_camera_tests
    finish_camera_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/update_finish_camera.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(finish_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(finish_camera_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(finish_camera_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(finish_camera_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME finish_camera COMMAND finish_camera_tests)
set_tests_properties(finish_camera PROPERTIES LABELS functional)

# The three AI walks over the course's own crest, speed and racing-line tables.
add_executable(car_ai_tests
    car_ai_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_ai.c)
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
set_tests_properties(car_ai PROPERTIES LABELS functional)

add_executable(car_motion_tests
    car_motion_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_ai.c)
target_include_directories(car_motion_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_motion_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_motion_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_motion_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_motion_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_motion_tests PRIVATE psyz)
add_test(NAME car_motion COMMAND car_motion_tests)
set_tests_properties(car_motion PROPERTIES LABELS functional)

add_executable(car_body_roll_tests
    car_body_roll_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_body_roll.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(car_body_roll_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_body_roll_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_body_roll_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_body_roll_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_body_roll_tests PRIVATE psyz)
add_test(NAME car_body_roll COMMAND car_body_roll_tests)
set_tests_properties(car_body_roll PROPERTIES LABELS functional)

add_executable(car_knockback_tests
    car_knockback_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_knockback.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c)
target_include_directories(car_knockback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_knockback_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_knockback_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_knockback_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(car_knockback_tests PRIVATE psyz)
add_test(NAME car_knockback COMMAND car_knockback_tests)
set_tests_properties(car_knockback PROPERTIES LABELS functional)

add_executable(car_lateral_offset_tests
    car_lateral_offset_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/clamp_car_lateral_offset.c)
target_include_directories(car_lateral_offset_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_lateral_offset_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_lateral_offset_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_lateral_offset_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(car_lateral_offset_tests PRIVATE psyz)
add_test(NAME car_lateral_offset COMMAND car_lateral_offset_tests)
set_tests_properties(car_lateral_offset PROPERTIES LABELS unit)

add_executable(paint_color_tests
    paint_color_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/paint_color.c)
target_include_directories(paint_color_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(paint_color_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(paint_color_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(paint_color_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(paint_color_tests PRIVATE psyz)
add_test(NAME paint_color COMMAND paint_color_tests)
set_tests_properties(paint_color PROPERTIES LABELS unit)

add_executable(car_spatial_queries_tests
    car_spatial_queries_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_spatial_queries.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(car_spatial_queries_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_spatial_queries_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_spatial_queries_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_spatial_queries_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_spatial_queries_tests PRIVATE psyz)
add_test(NAME car_spatial_queries COMMAND car_spatial_queries_tests)
set_tests_properties(car_spatial_queries PROPERTIES LABELS unit)

add_executable(car_performance_setup_tests
    car_performance_setup_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_performance_setup.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(car_performance_setup_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_performance_setup_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_performance_setup_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_performance_setup_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_performance_setup_tests PRIVATE psyz)
add_test(NAME car_performance_setup COMMAND car_performance_setup_tests)

add_executable(rival_car_init_tests
    rival_car_init_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_ai_setup.c)
target_include_directories(rival_car_init_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(rival_car_init_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(rival_car_init_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(rival_car_init_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rival_car_init_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME rival_car_init COMMAND rival_car_init_tests)
set_tests_properties(rival_car_init PROPERTIES LABELS functional)
set_tests_properties(car_performance_setup PROPERTIES LABELS unit)

# The drivetrain branches a smoke run never reaches: tall gears and mid-shift.
add_executable(drivetrain_tests
    drivetrain_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c)
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
set_tests_properties(drivetrain PROPERTIES LABELS functional)

add_executable(drivetrain_sweep_tests
    drivetrain_sweep_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c)
target_include_directories(drivetrain_sweep_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(drivetrain_sweep_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(drivetrain_sweep_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(drivetrain_sweep_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(drivetrain_sweep_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
target_link_libraries(drivetrain_sweep_tests PRIVATE psyz)
add_test(NAME drivetrain_sweep COMMAND drivetrain_sweep_tests)
set_tests_properties(drivetrain_sweep PROPERTIES LABELS functional)

# Where a car lands on the track, swept over a course built for the purpose.
add_executable(advance_car_position_tests
    advance_car_position_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/advance_car_position.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rot_matrix.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(advance_car_position_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(advance_car_position_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(advance_car_position_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(advance_car_position_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(advance_car_position_tests PRIVATE psyz)
add_test(NAME advance_car_position COMMAND advance_car_position_tests)
set_tests_properties(advance_car_position PROPERTIES LABELS functional)

add_executable(steer_to_track_line_tests
    steer_to_track_line_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/steer_car_to_track_line.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rot_matrix.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(steer_to_track_line_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(steer_to_track_line_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(steer_to_track_line_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(steer_to_track_line_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(steer_to_track_line_tests PRIVATE psyz)
add_test(NAME steer_to_track_line COMMAND steer_to_track_line_tests)
set_tests_properties(steer_to_track_line PROPERTIES LABELS functional)

add_executable(car_motion_handlers_tests
    car_motion_handlers_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_motion_handlers.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/steer_car_to_track_line.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/advance_car_position.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rot_matrix.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(car_motion_handlers_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_motion_handlers_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_motion_handlers_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_motion_handlers_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_motion_handlers_tests PRIVATE psyz)
add_test(NAME car_motion_handlers COMMAND car_motion_handlers_tests)
set_tests_properties(car_motion_handlers PROPERTIES LABELS functional)

add_executable(find_track_segment_tests
    find_track_segment_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/find_track_segment.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(find_track_segment_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(find_track_segment_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(find_track_segment_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(find_track_segment_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(find_track_segment_tests PRIVATE psyz)
add_test(NAME find_track_segment COMMAND find_track_segment_tests)
set_tests_properties(find_track_segment PROPERTIES LABELS functional)

add_executable(find_nearest_track_camera_tests
    find_nearest_track_camera_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track/find_nearest_track_camera.c)
target_include_directories(find_nearest_track_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(find_nearest_track_camera_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(find_nearest_track_camera_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(find_nearest_track_camera_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME find_nearest_track_camera COMMAND find_nearest_track_camera_tests)
set_tests_properties(find_nearest_track_camera PROPERTIES LABELS unit)

add_executable(car_track_state_tests
    car_track_state_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_track_state.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/arc_measure.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rot_matrix.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c
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
set_tests_properties(car_track_state PROPERTIES LABELS functional)

add_executable(track_contact_tests
    track_contact_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/track_contact.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/arc_measure.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rot_matrix.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(track_contact_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(track_contact_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(track_contact_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(track_contact_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(track_contact_tests PRIVATE psyz)
add_test(NAME track_contact COMMAND track_contact_tests)
set_tests_properties(track_contact PROPERTIES LABELS functional)

add_executable(collision_quads_tests
    collision_quads_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/collision_quads.c)
target_include_directories(collision_quads_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(collision_quads_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(collision_quads_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(collision_quads_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME collision_quads COMMAND collision_quads_tests)
set_tests_properties(collision_quads PROPERTIES LABELS unit)

add_executable(starting_grid_tests
    starting_grid_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/starting_grid.c)
target_include_directories(starting_grid_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(starting_grid_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(starting_grid_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(starting_grid_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME starting_grid COMMAND starting_grid_tests)
set_tests_properties(starting_grid PROPERTIES LABELS functional)

# The gearbox, on its own rather than through a whole race.
add_executable(gear_shift_tests
    gear_shift_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/gear_shift.c)
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
set_tests_properties(gear_shift PROPERTIES LABELS functional)

add_executable(rival_collision_tests
    rival_collision_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_collision.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/collision_quads.c)
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
set_tests_properties(rival_collision PROPERTIES LABELS functional)

add_executable(player_car_collision_tests
    player_car_collision_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_car_collision.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/collision_quads.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_spatial_queries.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/rot_matrix.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/gte_matrix.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(player_car_collision_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(player_car_collision_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(player_car_collision_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(player_car_collision_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(player_car_collision_tests PRIVATE psyz)
add_test(NAME player_car_collision COMMAND player_car_collision_tests)
set_tests_properties(player_car_collision PROPERTIES LABELS functional)
endif()

if(TARGET rage-racer-smoke)
    add_test(
        NAME direct_boot_car_setup
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_direct_boot_car_setup.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(direct_boot_car_setup PROPERTIES LABELS e2e)
    set_tests_properties(direct_boot_car_setup PROPERTIES TIMEOUT 105)
    add_test(NAME retire_camera COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_retire_camera.py
        $<TARGET_FILE:rage-racer-smoke> ${CMAKE_SOURCE_DIR})
set_tests_properties(retire_camera PROPERTIES LABELS e2e)
    set_tests_properties(retire_camera PROPERTIES TIMEOUT 165)
endif()

add_executable(traffic_avoidance_tests
    traffic_avoidance_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_ai.c
    $<TARGET_OBJECTS:rage-host-state>)
target_include_directories(traffic_avoidance_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(traffic_avoidance_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(traffic_avoidance_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(traffic_avoidance_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(traffic_avoidance_tests PRIVATE psyz)
add_test(NAME traffic_avoidance COMMAND traffic_avoidance_tests)
set_tests_properties(traffic_avoidance PROPERTIES LABELS functional)

add_executable(lap_progress_tests
    lap_progress_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/lap_progress.c)
target_include_directories(lap_progress_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(lap_progress_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(lap_progress_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(lap_progress_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(lap_progress_tests PRIVATE psyz)
add_test(NAME lap_progress COMMAND lap_progress_tests)
set_tests_properties(lap_progress PROPERTIES LABELS unit)
