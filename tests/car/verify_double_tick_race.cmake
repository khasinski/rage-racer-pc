if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

# Drives the same race with the accelerator held, once at the retail step and
# once with game.tick_rate=double, and compares where the car is after the
# same amount of game time. Double steps the physics once per video field, so
# its tick N is retail tick N / 2.
function(drive_race rate output)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
            SDL_AUDIODRIVER=dummy
            RAGE_PORT_SCENARIO=1
            RAGE_PORT_SCENARIO_MODE=1
            RAGE_PORT_SCENARIO_SERIES=1
            RAGE_PORT_SCENARIO_CLASS=4
            RAGE_PORT_SCENARIO_COURSE=3
            RAGE_PORT_SCENARIO_CAR=12
            RAGE_PORT_SMOKE_FRAMES=6000
            RAGE_PORT_SMOKE_STOP_SCENE=12
            RAGE_PORT_SMOKE_STOP_SCENE_TIMER=450
            RAGE_PORT_RAW_INPUT_SCRIPT=1-6000:CROSS
            "${GAME}"
            --set video.renderer=classic
            --set race.transmission=automatic
            --set start.freeze=false
            --set diagnostics.race_trace=1
            --set game.tick_rate=${rate}
        WORKING_DIRECTORY "${SOURCE}"
        TIMEOUT 100
        RESULT_VARIABLE result
        OUTPUT_VARIABLE out
        ERROR_VARIABLE error)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "race failed (${rate}, ${result}):\n${out}${error}")
    endif()
    set(${output} "${error}" PARENT_SCOPE)
endfunction()

function(car_at log tick x_out z_out mult_out)
    if(NOT log MATCHES "racetrace frame=[0-9]+ tick=${tick} mult=([0-9]+) x=(-?[0-9]+) z=(-?[0-9]+) ")
        message(FATAL_ERROR "no trace for tick ${tick}")
    endif()
    set(${mult_out} ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(${x_out} ${CMAKE_MATCH_2} PARENT_SCOPE)
    set(${z_out} ${CMAKE_MATCH_3} PARENT_SCOPE)
endfunction()

function(abs_diff a b out)
    math(EXPR difference "${a} - ${b}")
    if(difference LESS 0)
        math(EXPR difference "-(${difference})")
    endif()
    set(${out} ${difference} PARENT_SCOPE)
endfunction()

drive_race(retail retail_log)
drive_race(double double_log)

if(NOT double_log MATCHES "mult=2")
    message(FATAL_ERROR "game.tick_rate=double never ran a double-rate tick")
endif()
if(retail_log MATCHES "mult=2")
    message(FATAL_ERROR "the retail run used double-rate ticks")
endif()

# After the standing start the car is moving: compare 6.6 s into the race.
car_at("${retail_log}" 200 retail_x retail_z retail_mult)
car_at("${double_log}" 400 double_x double_z double_mult)
if(NOT retail_mult EQUAL 1 OR NOT double_mult EQUAL 2)
    message(FATAL_ERROR "unexpected tick rates: ${retail_mult}, ${double_mult}")
endif()
if(retail_x EQUAL 39373)
    message(FATAL_ERROR "the retail car never left the start")
endif()
abs_diff(${retail_x} ${double_x} dx)
abs_diff(${retail_z} ${double_z} dz)
if(dx GREATER 300 OR dz GREATER 300)
    message(FATAL_ERROR
        "double-rate car drifted from retail: retail=(${retail_x},${retail_z}) "
        "double=(${double_x},${double_z})")
endif()
message(STATUS "double-rate car at (${double_x},${double_z}) vs retail "
               "(${retail_x},${retail_z}) after the same game time")
