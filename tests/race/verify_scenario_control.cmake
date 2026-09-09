if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

set(common_environment
    SDL_AUDIODRIVER=dummy RAGE_PORT_SCENARIO=1
    RAGE_PORT_SCENARIO_MODE=99 RAGE_PORT_SCENARIO_SERIES=99
    RAGE_PORT_SCENARIO_CLASS=99 RAGE_PORT_SCENARIO_COURSE=99
    RAGE_PORT_SCENARIO_CAR=99 RAGE_PORT_SCENARIO_GRID=0,1,2
    RAGE_PORT_SMOKE_FRAMES=2600 RAGE_PORT_SMOKE_STOP_SCENE=12
    RAGE_PORT_SMOKE_STOP_SCENE_TIMER=1)
execute_process(COMMAND ${CMAKE_COMMAND} -E env ${common_environment}
    RAGE_PORT_MODS_DIRECTORY= "${GAME}" --set boot.direct=false
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 135 RESULT_VARIABLE menu_result
    OUTPUT_VARIABLE menu_output ERROR_VARIABLE menu_error)
set(menu_log "${menu_output}${menu_error}")
if(NOT menu_result EQUAL 0)
    message(FATAL_ERROR "Scenario menu route failed (${menu_result}):\n${menu_log}")
endif()
foreach(required "ignoring invalid race.mode=99" "ignoring invalid race.grid"
    "scenario confirm scene=4 phase=0" "scenario confirm scene=8" "smoke synchronized stop")
    string(FIND "${menu_log}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Scenario menu route missed ${required}:\n${menu_log}")
    endif()
endforeach()

execute_process(COMMAND ${CMAKE_COMMAND} -E env ${common_environment}
    "${GAME}"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 135 RESULT_VARIABLE direct_result
    OUTPUT_VARIABLE direct_output ERROR_VARIABLE direct_error)
set(direct_log "${direct_output}${direct_error}")
if(NOT direct_result EQUAL 0)
    message(FATAL_ERROR "Scenario direct route failed (${direct_result}):\n${direct_log}")
endif()
foreach(required "scenario direct boot entered the race" "smoke synchronized stop")
    string(FIND "${direct_log}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Scenario direct route missed ${required}:\n${direct_log}")
    endif()
endforeach()
message(STATUS "Scenario validation and menu/direct routes passed")
