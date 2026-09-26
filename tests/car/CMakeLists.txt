add_executable(car_shift_tests car_shift_tests.c)
target_include_directories(car_shift_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(car_shift_tests PRIVATE rage-sim)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_shift_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME car_shift COMMAND car_shift_tests)
set_tests_properties(car_shift PROPERTIES LABELS unit)

add_test(NAME native_car_names
    COMMAND source_contract_tests native_car_names ${CMAKE_SOURCE_DIR})

set_tests_properties(native_car_names PROPERTIES LABELS functional)

add_executable(car_catalog_tests
    car_catalog_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/host_state_car_catalog.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_catalog.c)
target_include_directories(car_catalog_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_catalog_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_catalog_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_catalog_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME car_catalog COMMAND car_catalog_tests)
set_tests_properties(car_catalog PROPERTIES LABELS unit)

add_executable(regional_speed_units_tests
    regional_speed_units_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/speed_display.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_ai_client.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_speed_client.c
)
target_include_directories(regional_speed_units_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(regional_speed_units_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(regional_speed_units_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(regional_speed_units_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(regional_speed_units_tests PRIVATE rage-sim)
add_test(NAME regional_speed_units COMMAND regional_speed_units_tests)
set_tests_properties(regional_speed_units PROPERTIES LABELS unit)

add_executable(chase_camera_tests
    chase_camera_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/chase_camera.c)
target_include_directories(chase_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port
    ${CMAKE_SOURCE_DIR}/src/port/include)
if(UNIX AND NOT APPLE)
    target_link_libraries(chase_camera_tests PRIVATE m)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(chase_camera_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME chase_camera COMMAND chase_camera_tests)
add_test(NAME chase_camera_invalid COMMAND chase_camera_tests)
set_tests_properties(chase_camera_invalid PROPERTIES
    ENVIRONMENT "CHASE_TEST_VALUE=nan")

set_tests_properties(chase_camera chase_camera_invalid PROPERTIES
    LABELS functional)

add_executable(player_tachometer_tests
    player_tachometer_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_tachometer.c)
target_include_directories(player_tachometer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(player_tachometer_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(player_tachometer_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(player_tachometer_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME player_tachometer COMMAND player_tachometer_tests)
set_tests_properties(player_tachometer PROPERTIES LABELS unit)

add_executable(player_input_tests
    player_input_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_input.c)
target_include_directories(player_input_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(player_input_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(player_input_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(player_input_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_input_tests PRIVATE rage-sim psyz)
add_test(NAME player_input COMMAND player_input_tests)
set_tests_properties(player_input PROPERTIES LABELS unit)

add_executable(player_jump_tests
    player_jump_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_jump_audio.c)
target_include_directories(player_jump_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(player_jump_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(player_jump_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(player_jump_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_jump_tests PRIVATE rage-sim)
add_test(NAME player_jump COMMAND player_jump_tests)
set_tests_properties(player_jump PROPERTIES LABELS unit)

add_executable(player_engine_presentation_tests
    player_engine_presentation_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_engine_presentation.c)
target_include_directories(player_engine_presentation_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(player_engine_presentation_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(player_engine_presentation_tests PRIVATE rage-sim)
add_test(NAME player_engine_presentation
    COMMAND player_engine_presentation_tests)
set_tests_properties(player_engine_presentation PROPERTIES LABELS unit)

add_executable(player_track_limits_tests player_track_limits_tests.c)
target_include_directories(player_track_limits_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(player_track_limits_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_track_limits_tests PRIVATE rage-sim psyz)
add_test(NAME player_track_limits COMMAND player_track_limits_tests)
set_tests_properties(player_track_limits PROPERTIES LABELS unit)

add_executable(player_contact_response_tests
    player_contact_response_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_contact_audio.c)
target_include_directories(player_contact_response_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(player_contact_response_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(player_contact_response_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(player_contact_response_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_contact_response_tests PRIVATE rage-sim)
add_test(NAME player_contact_response COMMAND player_contact_response_tests)
set_tests_properties(player_contact_response PROPERTIES LABELS unit)

add_executable(player_controls_tests player_controls_tests.c)
target_include_directories(player_controls_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(player_controls_tests PRIVATE rage-sim)
if(NOT MSVC)
    target_compile_options(player_controls_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME player_controls COMMAND player_controls_tests)
set_tests_properties(player_controls PROPERTIES LABELS unit)

add_executable(player_body_offset_tests player_body_offset_tests.c)
target_include_directories(player_body_offset_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(player_body_offset_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_body_offset_tests PRIVATE rage-sim psyz)
add_test(NAME player_body_offset COMMAND player_body_offset_tests)
set_tests_properties(player_body_offset PROPERTIES LABELS unit)

add_executable(player_track_contact_tests player_track_contact_tests.c)
target_include_directories(player_track_contact_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(player_track_contact_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_track_contact_tests PRIVATE rage-sim psyz)
add_test(NAME player_track_contact COMMAND player_track_contact_tests)
set_tests_properties(player_track_contact PROPERTIES LABELS unit)

add_executable(update_player_car_tests
    update_player_car_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_player_car.c)
target_include_directories(update_player_car_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(update_player_car_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(update_player_car_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(update_player_car_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(update_player_car_tests PRIVATE rage-sim)
add_test(NAME update_player_car COMMAND update_player_car_tests)
set_tests_properties(update_player_car PROPERTIES LABELS functional)

add_executable(player_car_init_tests
    player_car_init_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_car_init.c)
target_include_directories(player_car_init_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(player_car_init_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(player_car_init_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(player_car_init_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_car_init_tests PRIVATE rage-sim)
add_test(NAME player_car_init COMMAND player_car_init_tests)
set_tests_properties(player_car_init PROPERTIES LABELS functional)

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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
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
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/track
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(track_camera_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(track_camera_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(track_camera_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
target_link_libraries(track_camera_tests PRIVATE rage-sim psyz)
if(NOT MSVC)
    target_compile_options(track_camera_tests PRIVATE ${RAGE_NONFATAL_WARNING_OPTIONS})
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME finish_camera COMMAND finish_camera_tests)
set_tests_properties(finish_camera PROPERTIES LABELS functional)

# The three AI walks over the course's own crest, speed and racing-line tables.
add_executable(car_ai_tests
    car_ai_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_speed_client.c)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_ai_tests PRIVATE ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(car_ai_tests PRIVATE rage-sim psyz)
add_test(NAME car_ai COMMAND car_ai_tests)
set_tests_properties(car_ai PROPERTIES LABELS functional)

add_executable(car_motion_tests
    car_motion_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_slide_client.c)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_motion_tests PRIVATE ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(car_motion_tests PRIVATE rage-sim psyz)
add_test(NAME car_motion COMMAND car_motion_tests)
set_tests_properties(car_motion PROPERTIES LABELS functional)

add_executable(car_body_roll_tests car_body_roll_tests.c)
target_include_directories(car_body_roll_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(car_body_roll_tests PRIVATE rage-sim psyz)
if(NOT MSVC)
    target_compile_options(car_body_roll_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME car_body_roll COMMAND car_body_roll_tests)
set_tests_properties(car_body_roll PROPERTIES LABELS unit)

add_executable(car_knockback_tests
    car_knockback_tests.c
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(car_knockback_tests PRIVATE rage-sim psyz)
add_test(NAME car_knockback COMMAND car_knockback_tests)
set_tests_properties(car_knockback PROPERTIES LABELS functional)

add_executable(car_lateral_offset_tests car_lateral_offset_tests.c)
target_include_directories(car_lateral_offset_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(car_lateral_offset_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(car_lateral_offset_tests PRIVATE rage-sim)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(paint_color_tests PRIVATE psyz)
add_test(NAME paint_color COMMAND paint_color_tests)
set_tests_properties(paint_color PROPERTIES LABELS unit)

add_executable(car_spatial_queries_tests car_spatial_queries_tests.c)
target_include_directories(car_spatial_queries_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_spatial_queries_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(car_spatial_queries_tests PRIVATE rage-sim)
add_test(NAME car_spatial_queries COMMAND car_spatial_queries_tests)
set_tests_properties(car_spatial_queries PROPERTIES LABELS unit)

add_executable(car_performance_setup_tests car_performance_setup_tests.c)
target_include_directories(car_performance_setup_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(car_performance_setup_tests PRIVATE rage-sim)
if(NOT MSVC)
    target_compile_options(car_performance_setup_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME car_performance_setup COMMAND car_performance_setup_tests)
set_tests_properties(car_performance_setup PROPERTIES LABELS unit)

add_executable(rival_ai_setup_tests
    rival_ai_setup_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_ai_client.c)
target_include_directories(rival_ai_setup_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(rival_ai_setup_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(rival_ai_setup_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(rival_ai_setup_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rival_ai_setup_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(rival_ai_setup_tests PRIVATE rage-sim)
add_test(NAME rival_ai_setup COMMAND rival_ai_setup_tests)
set_tests_properties(rival_ai_setup PROPERTIES LABELS functional)
set_tests_properties(car_performance_setup PROPERTIES LABELS unit)

add_executable(rival_runtime_init_tests
    rival_runtime_init_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_init_client.c)
target_include_directories(rival_runtime_init_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(rival_runtime_init_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(rival_runtime_init_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(rival_runtime_init_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(rival_runtime_init_tests PRIVATE rage-sim)
add_test(NAME rival_runtime_init COMMAND rival_runtime_init_tests)
set_tests_properties(rival_runtime_init PROPERTIES LABELS unit)

# The drivetrain branches a smoke run never reaches: tall gears and mid-shift.
add_executable(drive_context_tests drive_context_tests.c)
target_include_directories(drive_context_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(drive_context_tests PRIVATE rage-sim psyz)
if(NOT MSVC)
    target_compile_options(drive_context_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME drive_context COMMAND drive_context_tests)
set_tests_properties(drive_context PROPERTIES LABELS unit)

add_executable(drivetrain_step_tests drivetrain_step_tests.c)
target_include_directories(drivetrain_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(drivetrain_step_tests PRIVATE rage-sim psyz)
if(NOT MSVC)
    target_compile_options(drivetrain_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME drivetrain_step COMMAND drivetrain_step_tests)
set_tests_properties(drivetrain_step PROPERTIES LABELS unit)

add_executable(drivetrain_tests
    drivetrain_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c
)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(drivetrain_tests PRIVATE ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(drivetrain_tests PRIVATE rage-sim psyz)
add_test(NAME drivetrain COMMAND drivetrain_tests)
set_tests_properties(drivetrain PROPERTIES LABELS functional)

add_executable(drivetrain_sweep_tests
    drivetrain_sweep_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c
)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(drivetrain_sweep_tests PRIVATE ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(drivetrain_sweep_tests PRIVATE rage-sim psyz)
add_test(NAME drivetrain_sweep COMMAND drivetrain_sweep_tests)
set_tests_properties(drivetrain_sweep PROPERTIES LABELS functional)

# Travel heading and speed over a sweep of body directions and acceleration.
add_executable(car_travel_velocity_tests advance_car_position_tests.c)
target_include_directories(car_travel_velocity_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(car_travel_velocity_tests PRIVATE rage-sim psyz)
if(NOT MSVC)
    target_compile_options(car_travel_velocity_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME car_travel_velocity COMMAND car_travel_velocity_tests)
set_tests_properties(car_travel_velocity PROPERTIES LABELS functional)


add_executable(steer_to_track_line_tests steer_to_track_line_tests.c)
target_include_directories(steer_to_track_line_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(steer_to_track_line_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(steer_to_track_line_tests PRIVATE rage-sim psyz)
add_test(NAME steer_to_track_line COMMAND steer_to_track_line_tests)
set_tests_properties(steer_to_track_line PROPERTIES LABELS functional)

add_executable(airborne_step_tests airborne_step_tests.c)
target_include_directories(airborne_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(airborne_step_tests PRIVATE rage-sim psyz)
if(NOT MSVC)
    target_compile_options(airborne_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME airborne_step COMMAND airborne_step_tests)
set_tests_properties(airborne_step PROPERTIES LABELS unit)

add_executable(car_motion_handlers_tests
    car_motion_handlers_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_driving_audio.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_launch_audio.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_airborne_audio.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_standing_start_audio.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/track_steering_target.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/steer_car_to_track_line.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/angle_math.c
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        "${RAGE_QUOTE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include"
        "${RAGE_QUOTE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/include"
        ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(car_motion_handlers_tests PRIVATE rage-sim psyz)
add_test(NAME car_motion_handlers COMMAND car_motion_handlers_tests)
set_tests_properties(car_motion_handlers PROPERTIES LABELS functional)

add_executable(find_track_segment_tests find_track_segment_tests.c)
target_include_directories(find_track_segment_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(find_track_segment_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(find_track_segment_tests PRIVATE rage-sim psyz)
add_test(NAME find_track_segment COMMAND find_track_segment_tests)
set_tests_properties(find_track_segment PROPERTIES LABELS functional)

add_executable(arc_measure_tests
    arc_measure_tests.c)
target_include_directories(arc_measure_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(arc_measure_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(arc_measure_tests PRIVATE rage-sim psyz)
add_test(NAME arc_measure COMMAND arc_measure_tests)
set_tests_properties(arc_measure PROPERTIES LABELS unit)

add_executable(car_track_math_tests
    car_track_math_tests.c)
target_include_directories(car_track_math_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_track_math_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(car_track_math_tests PRIVATE rage-sim)
add_test(NAME car_track_math COMMAND car_track_math_tests)
set_tests_properties(car_track_math PROPERTIES LABELS unit)

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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME find_nearest_track_camera COMMAND find_nearest_track_camera_tests)
set_tests_properties(find_nearest_track_camera PROPERTIES LABELS unit)

add_executable(car_track_state_tests car_track_state_tests.c)
target_include_directories(car_track_state_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_track_state_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(car_track_state_tests PRIVATE rage-sim psyz)
add_test(NAME car_track_state COMMAND car_track_state_tests)
set_tests_properties(car_track_state PROPERTIES LABELS functional)

add_executable(replay_track_reconstruction_tests replay_track_reconstruction_tests.c)
target_include_directories(replay_track_reconstruction_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(replay_track_reconstruction_tests PRIVATE
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(replay_track_reconstruction_tests PRIVATE rage-sim psyz)
add_test(NAME replay_track_reconstruction
         COMMAND replay_track_reconstruction_tests)
set_tests_properties(replay_track_reconstruction PROPERTIES LABELS functional)

add_executable(collision_quads_tests
    collision_quads_tests.c)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(collision_quads_tests PRIVATE rage-sim psyz)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME starting_grid COMMAND starting_grid_tests)
set_tests_properties(starting_grid PROPERTIES LABELS functional)

add_executable(player_tilt_tests player_tilt_tests.c)
target_include_directories(player_tilt_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(player_tilt_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(player_tilt_tests PRIVATE rage-sim)
add_test(NAME player_tilt COMMAND player_tilt_tests)
set_tests_properties(player_tilt PROPERTIES LABELS unit)

add_executable(draw_cars_tests
    draw_cars_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/draw_cars.c)
target_include_directories(draw_cars_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(draw_cars_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(draw_cars_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(draw_cars_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME draw_cars COMMAND draw_cars_tests)
set_tests_properties(draw_cars PROPERTIES LABELS unit)

add_executable(car_body_kick_tests car_body_kick_tests.c)
target_include_directories(car_body_kick_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_body_kick_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(car_body_kick_tests PRIVATE rage-sim psyz)
add_test(NAME car_body_kick COMMAND car_body_kick_tests)
set_tests_properties(car_body_kick PROPERTIES LABELS unit)

add_executable(car_driving_tests
    car_driving_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_driving_audio.c)
target_include_directories(car_driving_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_driving_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_driving_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_driving_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(car_driving_tests PRIVATE rage-sim)
add_test(NAME car_driving COMMAND car_driving_tests)
set_tests_properties(car_driving PROPERTIES LABELS unit)

# The gearbox, on its own rather than through a whole race.
add_executable(gear_shift_tests gear_shift_tests.c)
target_include_directories(gear_shift_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(gear_shift_tests PRIVATE rage-sim)
if(NOT MSVC)
    target_compile_options(gear_shift_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME gear_shift COMMAND gear_shift_tests)
set_tests_properties(gear_shift PROPERTIES LABELS unit)

add_executable(rival_collision_tests
    rival_collision_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_collision_client.c)
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(rival_collision_tests PRIVATE
        ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(rival_collision_tests PRIVATE rage-sim psyz)
add_test(NAME rival_collision COMMAND rival_collision_tests)
set_tests_properties(rival_collision PROPERTIES LABELS functional)

add_executable(player_car_collision_tests
    player_car_collision_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_car_collision.c
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(player_car_collision_tests PRIVATE rage-sim psyz)
add_test(NAME player_car_collision COMMAND player_car_collision_tests)
set_tests_properties(player_car_collision PROPERTIES LABELS functional)
endif()

if(TARGET rage-racer-smoke)
add_test(NAME scenario_car_setup COMMAND ${CMAKE_COMMAND}
        -DGAME=$<TARGET_FILE:rage-racer-smoke>
        -DSOURCE=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_direct_boot_car_setup.cmake)
set_tests_properties(scenario_car_setup PROPERTIES LABELS e2e)
    set_tests_properties(scenario_car_setup PROPERTIES TIMEOUT 105)
    add_test(NAME retire_camera COMMAND ${CMAKE_COMMAND}
        -DGAME=$<TARGET_FILE:rage-racer-smoke>
        -DSOURCE=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_retire_camera.cmake)
set_tests_properties(retire_camera PROPERTIES LABELS e2e)
    set_tests_properties(retire_camera PROPERTIES TIMEOUT 165)
endif()

add_executable(traffic_avoidance_tests
    traffic_avoidance_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_traffic_client.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_ranking.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_rubber_band.c
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
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        "${RAGE_QUOTE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include"
        "${RAGE_QUOTE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/include"
        ${RAGE_NONFATAL_WARNING_OPTIONS})
endif()
target_link_libraries(traffic_avoidance_tests PRIVATE psyz rage-sim)
add_test(NAME traffic_avoidance COMMAND traffic_avoidance_tests)
set_tests_properties(traffic_avoidance PROPERTIES LABELS functional)

add_executable(lap_progress_tests lap_progress_tests.c)
target_include_directories(lap_progress_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(lap_progress_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(lap_progress_tests PRIVATE rage-sim)
add_test(NAME lap_progress COMMAND lap_progress_tests)
set_tests_properties(lap_progress PROPERTIES LABELS unit)

add_executable(standing_start_setup_tests standing_start_setup_tests.c)
target_include_directories(standing_start_setup_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(standing_start_setup_tests PRIVATE rage-sim psyz)
if(NOT MSVC)
    target_compile_options(standing_start_setup_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME standing_start_setup COMMAND standing_start_setup_tests)
set_tests_properties(standing_start_setup PROPERTIES LABELS unit)

add_executable(seed_finish_camera_tests
    seed_finish_camera_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/seed_finish_camera.c)
target_include_directories(seed_finish_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(seed_finish_camera_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(seed_finish_camera_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(seed_finish_camera_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(seed_finish_camera_tests PRIVATE psyz)
add_test(NAME seed_finish_camera COMMAND seed_finish_camera_tests)
set_tests_properties(seed_finish_camera PROPERTIES LABELS unit)

add_executable(race_intro_camera_tests
    race_intro_camera_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/race_intro_camera.c)
target_include_directories(race_intro_camera_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(race_intro_camera_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(race_intro_camera_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(race_intro_camera_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(race_intro_camera_tests PRIVATE psyz)
add_test(NAME race_intro_camera COMMAND race_intro_camera_tests)
set_tests_properties(race_intro_camera PROPERTIES LABELS unit)

add_executable(steer_car_along_route_tests steer_car_along_route_tests.c)
target_include_directories(steer_car_along_route_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(steer_car_along_route_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(steer_car_along_route_tests PRIVATE rage-sim)
add_test(NAME steer_car_along_route COMMAND steer_car_along_route_tests)
set_tests_properties(steer_car_along_route PROPERTIES LABELS unit)

add_executable(rival_body_motion_tests
    rival_body_motion_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/car_wheel_rotation.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_speed_client.c)
target_include_directories(rival_body_motion_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(rival_body_motion_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(rival_body_motion_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(rival_body_motion_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(rival_body_motion_tests PRIVATE rage-sim psyz)
add_test(NAME rival_body_motion COMMAND rival_body_motion_tests)
set_tests_properties(rival_body_motion PROPERTIES LABELS unit)

add_executable(rival_car_movement_tests rival_car_movement_tests.c)
target_include_directories(rival_car_movement_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_car_movement_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_car_movement_tests PRIVATE rage-sim)
add_test(NAME rival_car_movement COMMAND rival_car_movement_tests)
set_tests_properties(rival_car_movement PROPERTIES LABELS functional)

add_executable(race_car_update_tests
    race_car_update_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/race_car_update.c)
target_include_directories(race_car_update_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(race_car_update_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(race_car_update_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(race_car_update_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        "${RAGE_QUOTE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include"
        "${RAGE_QUOTE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/include"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
add_test(NAME race_car_update COMMAND race_car_update_tests)
set_tests_properties(race_car_update PROPERTIES LABELS unit)

add_executable(rival_acceleration_tests rival_acceleration_tests.c)
target_include_directories(rival_acceleration_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_acceleration_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_acceleration_tests PRIVATE rage-sim)
add_test(NAME rival_acceleration COMMAND rival_acceleration_tests)
set_tests_properties(rival_acceleration PROPERTIES LABELS unit)

add_executable(rival_track_placement_tests
    rival_track_placement_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/rival_track_placement.c)
target_include_directories(rival_track_placement_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(rival_track_placement_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(rival_track_placement_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(rival_track_placement_tests PRIVATE
        "${RAGE_FORCE_INCLUDE_OPTION}${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(rival_track_placement_tests PRIVATE rage-sim psyz)
add_test(NAME rival_track_placement COMMAND rival_track_placement_tests)
set_tests_properties(rival_track_placement PROPERTIES LABELS unit)

add_executable(track_contact_step_tests track_contact_step_tests.c)
target_include_directories(track_contact_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(track_contact_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(track_contact_step_tests PRIVATE rage-sim psyz)
add_test(NAME track_contact_step COMMAND track_contact_step_tests)
set_tests_properties(track_contact_step PROPERTIES LABELS unit)

add_executable(jump_step_tests jump_step_tests.c)
target_include_directories(jump_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(jump_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(jump_step_tests PRIVATE rage-sim)
add_test(NAME jump_step COMMAND jump_step_tests)
set_tests_properties(jump_step PROPERTIES LABELS unit)

add_executable(collision_response_tests collision_response_tests.c)
target_include_directories(collision_response_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(collision_response_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(collision_response_tests PRIVATE rage-sim psyz)
add_test(NAME collision_response COMMAND collision_response_tests)
set_tests_properties(collision_response PROPERTIES LABELS unit)

add_executable(car_contact_tests car_contact_tests.c)
target_include_directories(car_contact_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(car_contact_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(car_contact_tests PRIVATE rage-sim psyz)
add_test(NAME car_contact COMMAND car_contact_tests)
set_tests_properties(car_contact PROPERTIES LABELS unit)

add_executable(contact_response_step_tests contact_response_step_tests.c)
target_include_directories(contact_response_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(contact_response_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(contact_response_step_tests PRIVATE rage-sim psyz)
add_test(NAME contact_response_step COMMAND contact_response_step_tests)
set_tests_properties(contact_response_step PROPERTIES LABELS unit)

add_executable(crest_step_tests crest_step_tests.c)
target_include_directories(crest_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(crest_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(crest_step_tests PRIVATE rage-sim)
add_test(NAME crest_step COMMAND crest_step_tests)
set_tests_properties(crest_step PROPERTIES LABELS unit)

add_executable(route_interpolation_tests route_interpolation_tests.c)
target_include_directories(route_interpolation_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(route_interpolation_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(route_interpolation_tests PRIVATE rage-sim)
add_test(NAME route_interpolation COMMAND route_interpolation_tests)
set_tests_properties(route_interpolation PROPERTIES LABELS unit)

add_executable(route_steering_tests route_steering_tests.c)
target_include_directories(route_steering_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(route_steering_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(route_steering_tests PRIVATE rage-sim psyz)
add_test(NAME route_steering COMMAND route_steering_tests)
set_tests_properties(route_steering PROPERTIES LABELS unit)

add_executable(launch_step_tests launch_step_tests.c)
target_include_directories(launch_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(launch_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(launch_step_tests PRIVATE rage-sim psyz)
add_test(NAME launch_step COMMAND launch_step_tests)
set_tests_properties(launch_step PROPERTIES LABELS unit)

add_executable(slide_step_tests slide_step_tests.c)
target_include_directories(slide_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(slide_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(slide_step_tests PRIVATE rage-sim)
add_test(NAME slide_step COMMAND slide_step_tests)
set_tests_properties(slide_step PROPERTIES LABELS unit)

add_executable(driver_input_tests driver_input_tests.c)
target_include_directories(driver_input_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(driver_input_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(driver_input_tests PRIVATE rage-sim psyz)
add_test(NAME driver_input COMMAND driver_input_tests)
set_tests_properties(driver_input PROPERTIES LABELS unit)

add_executable(driver_step_tests driver_step_tests.c)
target_include_directories(driver_step_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(driver_step_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(driver_step_tests PRIVATE rage-sim psyz)
add_test(NAME driver_step COMMAND driver_step_tests)
set_tests_properties(driver_step PROPERTIES LABELS unit)

add_executable(driver_contact_tests driver_contact_tests.c)
target_include_directories(driver_contact_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(driver_contact_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(driver_contact_tests PRIVATE rage-sim psyz)
add_test(NAME driver_contact COMMAND driver_contact_tests)
set_tests_properties(driver_contact PROPERTIES LABELS unit)

add_executable(driver_field_tests driver_field_tests.c)
target_include_directories(driver_field_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(driver_field_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(driver_field_tests PRIVATE rage-sim psyz)
add_test(NAME driver_field COMMAND driver_field_tests)
set_tests_properties(driver_field PROPERTIES LABELS unit)

add_executable(driver_init_tests driver_init_tests.c)
target_include_directories(driver_init_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(driver_init_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(driver_init_tests PRIVATE rage-sim psyz)
add_test(NAME driver_init COMMAND driver_init_tests)
set_tests_properties(driver_init PROPERTIES LABELS unit)

add_executable(triangle_area_tests triangle_area_tests.c)
target_include_directories(triangle_area_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(triangle_area_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(triangle_area_tests PRIVATE rage-sim psyz)
add_test(NAME triangle_area COMMAND triangle_area_tests)
set_tests_properties(triangle_area PROPERTIES LABELS unit)

add_executable(hull_rotation_tests hull_rotation_tests.c)
target_include_directories(hull_rotation_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hull_rotation_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(hull_rotation_tests PRIVATE rage-sim psyz)
add_test(NAME hull_rotation COMMAND hull_rotation_tests)
set_tests_properties(hull_rotation PROPERTIES LABELS unit)

add_executable(trig_tests trig_tests.c)
target_include_directories(trig_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(trig_tests PRIVATE TRIG_REFERENCE=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(trig_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
endif()
target_link_libraries(trig_tests PRIVATE rage-sim psyz)
add_test(NAME trig COMMAND trig_tests)
set_tests_properties(trig PROPERTIES LABELS unit)

add_executable(track_route_tests track_route_tests.c)
target_include_directories(track_route_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(track_route_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(track_route_tests PRIVATE rage-sim)
add_test(NAME track_route COMMAND track_route_tests)
set_tests_properties(track_route PROPERTIES LABELS unit)

add_executable(track_events_tests track_events_tests.c)
target_include_directories(track_events_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(track_events_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(track_events_tests PRIVATE rage-sim)
add_test(NAME track_events COMMAND track_events_tests)
set_tests_properties(track_events PROPERTIES LABELS unit)

add_executable(track_data_tests track_data_tests.c)
target_include_directories(track_data_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(track_data_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(track_data_tests PRIVATE rage-sim)
add_test(NAME track_data COMMAND track_data_tests)
set_tests_properties(track_data PROPERTIES LABELS unit)

add_executable(car_asset_tests car_asset_tests.c)
target_include_directories(car_asset_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(car_asset_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(car_asset_tests PRIVATE rage-sim)
add_test(NAME car_asset COMMAND car_asset_tests)
set_tests_properties(car_asset PROPERTIES LABELS unit)

include(${CMAKE_SOURCE_DIR}/tests/sim/retail_data.cmake)

add_executable(shift_pitch_tests shift_pitch_tests.c)
target_include_directories(shift_pitch_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(shift_pitch_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(shift_pitch_tests PRIVATE rage-sim)
add_test(NAME shift_pitch COMMAND shift_pitch_tests)
set_tests_properties(shift_pitch PROPERTIES LABELS unit)

add_executable(rival_config_tests rival_config_tests.c)
target_include_directories(rival_config_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_config_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_config_tests PRIVATE rage-sim)
add_test(NAME rival_config COMMAND rival_config_tests)
set_tests_properties(rival_config PROPERTIES LABELS unit)

add_executable(rival_speed_key_tests rival_speed_key_tests.c)
target_include_directories(rival_speed_key_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_speed_key_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_speed_key_tests PRIVATE rage-sim)
add_test(NAME rival_speed_key COMMAND rival_speed_key_tests)
set_tests_properties(rival_speed_key PROPERTIES LABELS unit)

add_executable(rival_target_tests rival_target_tests.c)
target_include_directories(rival_target_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_target_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_target_tests PRIVATE rage-sim)
add_test(NAME rival_target COMMAND rival_target_tests)
set_tests_properties(rival_target PROPERTIES LABELS unit)

add_executable(rival_line_tests rival_line_tests.c)
target_include_directories(rival_line_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_line_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_line_tests PRIVATE rage-sim)
add_test(NAME rival_line COMMAND rival_line_tests)
set_tests_properties(rival_line PROPERTIES LABELS unit)

add_executable(rival_finish_tests rival_finish_tests.c)
target_include_directories(rival_finish_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_finish_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_finish_tests PRIVATE rage-sim)
add_test(NAME rival_finish COMMAND rival_finish_tests)
set_tests_properties(rival_finish PROPERTIES LABELS unit)

add_executable(rival_place_tests rival_place_tests.c)
target_include_directories(rival_place_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_place_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_place_tests PRIVATE rage-sim)
add_test(NAME rival_place COMMAND rival_place_tests)
set_tests_properties(rival_place PROPERTIES LABELS unit)

add_executable(rival_init_tests rival_init_tests.c)
target_include_directories(rival_init_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(rival_init_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(rival_init_tests PRIVATE rage-sim)
add_test(NAME rival_init COMMAND rival_init_tests)
set_tests_properties(rival_init PROPERTIES LABELS unit)

add_executable(rival_traffic_tests rival_traffic_tests.c)
target_include_directories(rival_traffic_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(rival_traffic_tests PRIVATE rage-sim)
add_test(NAME rival_traffic COMMAND rival_traffic_tests)

add_executable(rival_contact_tests rival_contact_tests.c)
target_include_directories(rival_contact_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(rival_contact_tests PRIVATE rage-sim)
add_test(NAME rival_contact COMMAND rival_contact_tests)

foreach(test IN ITEMS race_data race_disc)
    add_executable(${test}_tests ${test}_tests.c)
    target_link_libraries(${test}_tests PRIVATE rage-data)
    target_compile_options(${test}_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
    add_test(NAME ${test} COMMAND ${test}_tests)
    set_tests_properties(${test} PROPERTIES LABELS unit)
endforeach()

add_executable(catalog_parse_tests catalog_parse_tests.c)
target_include_directories(catalog_parse_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(catalog_parse_tests PRIVATE rage-sim)
target_compile_options(catalog_parse_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME catalog_parse COMMAND catalog_parse_tests)
set_tests_properties(catalog_parse PROPERTIES LABELS unit)

add_executable(client_driver_tests client_driver_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_player_car.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c)
target_include_directories(client_driver_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(client_driver_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(client_driver_tests PRIVATE rage-sim)
add_test(NAME client_driver COMMAND client_driver_tests)
set_tests_properties(client_driver PROPERTIES LABELS unit)

add_executable(driver_cues_tests driver_cues_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_jump_audio.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/player_contact_audio.c)
target_include_directories(driver_cues_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(driver_cues_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME driver_cues COMMAND driver_cues_tests)
set_tests_properties(driver_cues PROPERTIES LABELS unit)

add_executable(engine_sound_tests engine_sound_tests.c)
target_include_directories(engine_sound_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(engine_sound_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(engine_sound_tests PRIVATE rage-sim)
add_test(NAME engine_sound COMMAND engine_sound_tests)
set_tests_properties(engine_sound PROPERTIES LABELS unit)

add_executable(race_grid_tests ${CMAKE_SOURCE_DIR}/tests/car/race_grid_tests.c)
target_include_directories(race_grid_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(race_grid_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(race_grid_tests PRIVATE rage-data)
add_test(NAME race_grid COMMAND race_grid_tests)
set_tests_properties(race_grid PROPERTIES LABELS unit)

add_executable(scene_blocks_tests ${CMAKE_SOURCE_DIR}/tests/car/scene_blocks_tests.c)
target_include_directories(scene_blocks_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(scene_blocks_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(scene_blocks_tests PRIVATE rage-sim)
add_test(NAME scene_blocks COMMAND scene_blocks_tests)
set_tests_properties(scene_blocks PROPERTIES LABELS unit)

add_executable(track_zone_tests track_zone_tests.c)
target_include_directories(track_zone_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(track_zone_tests PRIVATE rage-sim)
target_compile_options(track_zone_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
add_test(NAME track_zone COMMAND track_zone_tests)
set_tests_properties(track_zone PROPERTIES LABELS unit)
