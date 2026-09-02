add_executable(input_device_select_tests
    input_device_select_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/input_device_select.c)
target_include_directories(input_device_select_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(input_device_select_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME input_device_select COMMAND input_device_select_tests)

set_tests_properties(input_device_select PROPERTIES LABELS functional)
add_executable(axis_curve_tests axis_curve_tests.c ${CMAKE_SOURCE_DIR}/src/port/axis_curve.c)
target_include_directories(axis_curve_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port)
if(NOT MSVC)
    target_compile_options(axis_curve_tests PRIVATE -Wall -Wextra)
    target_link_libraries(axis_curve_tests PRIVATE m)
endif()
add_test(NAME axis_curve COMMAND axis_curve_tests)
set_tests_properties(axis_curve PROPERTIES LABELS functional)

add_executable(controller_config_begin_tests
    controller_config_begin_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/pad/begin_controller_config.c)
target_include_directories(controller_config_begin_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(controller_config_begin_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(controller_config_begin_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(controller_config_begin_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME controller_config_begin COMMAND controller_config_begin_tests)
set_tests_properties(controller_config_begin PROPERTIES LABELS unit)

# Everything UpdatePadState makes of a raw pad packet: the synthetic twist and
# pressure a digital pad gets, the NeGcon's thresholds, the steering shaping,
# auto-repeat and the two error paths.
if(TARGET rage-host-state)
    add_executable(pad_state_tests
        pad_state_tests.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/pad/init_pad.c
        $<TARGET_OBJECTS:rage-host-state>
    )
    target_include_directories(pad_state_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/port
        ${CMAKE_SOURCE_DIR}/src/port/include)
    if(NOT WIN32)
        target_compile_options(pad_state_tests PRIVATE
            "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
            "-iquote${CMAKE_SOURCE_DIR}/include")
    endif()
    target_link_libraries(pad_state_tests PRIVATE psyz)
    if(RAGE_ENABLE_SANITIZERS AND
       CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
        target_link_options(pad_state_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
    add_test(NAME pad_state COMMAND pad_state_tests)
    set_tests_properties(pad_state PROPERTIES LABELS functional)
endif()
# The port's half of the NeGcon (NegconTwist) against the game's half
# (UpdatePadState), which is the only place a calibration fault shows.
if(TARGET rage-host-state)
    add_executable(negcon_calibration_tests
        negcon_calibration_tests.c
        ${CMAKE_SOURCE_DIR}/src/port/axis_curve.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/pad/init_pad.c
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
    set_tests_properties(negcon_calibration PROPERTIES LABELS functional)
endif()

if(TARGET rage-racer-smoke)
    add_test(
        NAME keyboard_input
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_keyboard_input.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(keyboard_input PROPERTIES LABELS e2e)
    set_tests_properties(keyboard_input PROPERTIES TIMEOUT 60)
endif()
