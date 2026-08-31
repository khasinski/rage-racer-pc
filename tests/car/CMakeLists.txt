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
add_executable(track_camera_tests
    track_camera_tests.c
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
