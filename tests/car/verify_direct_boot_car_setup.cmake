if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        SDL_AUDIODRIVER=dummy
        RAGE_PORT_SCENARIO=1
        RAGE_PORT_SCENARIO_MODE=1
        RAGE_PORT_SCENARIO_SERIES=1
        RAGE_PORT_SCENARIO_CLASS=4
        RAGE_PORT_SCENARIO_COURSE=3
        RAGE_PORT_SCENARIO_CAR=12
        RAGE_PORT_SMOKE_FRAMES=1200
        RAGE_PORT_SMOKE_STOP_SCENE=12
        RAGE_PORT_SMOKE_STOP_SCENE_TIMER=650
        RAGE_PORT_RAW_INPUT_SCRIPT=1-1200:CROSS
        "${GAME}"
        --set video.renderer=classic
        --set race.transmission=automatic
        --set start.freeze=false
    WORKING_DIRECTORY "${SOURCE}"
    TIMEOUT 90
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "scenario car setup failed (${result}):\n${log}")
endif()
string(REGEX MATCHALL "scenario launch selection applied" launch_applications "${log}")
list(LENGTH launch_applications launch_application_count)
if(NOT launch_application_count EQUAL 1)
    message(FATAL_ERROR "scenario selection was applied ${launch_application_count} times:\n${log}")
endif()
if(NOT log MATCHES "Rage Racer smoke stopped.* speed=([0-9]+).* accelerator=([0-9]+).* race_phase=([0-9]+).* manual=([0-9]+)")
    message(FATAL_ERROR "missing final car state:\n${log}")
endif()
if(NOT CMAKE_MATCH_1 GREATER 0 OR NOT CMAKE_MATCH_2 GREATER 0 OR
   NOT CMAKE_MATCH_3 EQUAL 2 OR NOT CMAKE_MATCH_4 EQUAL 1)
    message(FATAL_ERROR "manual-only car did not drive:\n${log}")
endif()
message(STATUS "normal launch carries manual-only car setup into race physics")
