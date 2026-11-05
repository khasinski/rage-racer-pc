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
# The native main menu opens directly; confirmation occurs in MENU_OPEN (2),
# without the former Press Start / TITLE (0) step.
foreach(required "ignoring invalid race.mode=99" "ignoring invalid race.grid"
    "scenario confirm scene=4 phase=2" "scenario confirm scene=8" "smoke synchronized stop")
    string(FIND "${menu_log}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Scenario menu route missed ${required}:\n${menu_log}")
    endif()
endforeach()

execute_process(COMMAND ${CMAKE_COMMAND} -E env ${common_environment}
    "${GAME}" --set boot.direct=true
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 135 RESULT_VARIABLE direct_result
    OUTPUT_VARIABLE direct_output ERROR_VARIABLE direct_error)
set(direct_log "${direct_output}${direct_error}")
if(NOT direct_result EQUAL 0)
    message(FATAL_ERROR "Scenario direct boot route failed (${direct_result}):\n${direct_log}")
endif()
foreach(required "scenario boot=direct"
                 "scenario launch selection applied"
                 "scenario direct boot entered the race"
                 "smoke synchronized stop")
    string(FIND "${direct_log}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Scenario direct boot route missed ${required}:\n${direct_log}")
    endif()
endforeach()
message(STATUS "Scenario validation and menu/direct launch routes passed")

foreach(class 0 4 5)
    execute_process(COMMAND ${CMAKE_COMMAND} -E env PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
        "${GAME}" --scenario "${SOURCE}/race-scenario.ini" --set video.renderer=classic
        --set race.series=extra-gp --set "race.class=${class}" --set race.course=0
        --set stop.scene=12 --set stop.timer=1 --set run.frames=2000
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60 RESULT_VARIABLE extra_result
        OUTPUT_VARIABLE extra_output ERROR_VARIABLE extra_error)
    set(asset_series 1)
    if(class EQUAL 5)
        set(asset_series 0)
    endif()
    if(NOT extra_result EQUAL 0 OR
       NOT extra_error MATCHES "direct boot entered the race course=0 asset_series=${asset_series} selected_series=1" OR
       NOT extra_error MATCHES "scene=12 timer=1")
        message(FATAL_ERROR "Extra GP class ${class} direct boot failed: ${extra_output}${extra_error}")
    endif()
endforeach()

# Held input is anchored to race time, so faster menus or asynchronous asset
# loading cannot move steering/acceleration to a different point in the race.
execute_process(COMMAND ${CMAKE_COMMAND} -E env PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
    "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
    --set video.renderer=classic --set diagnostics.race_trace=true
    --set input.disable_host=true --set input.state_script=12@200-250:CROSS
    --set run.frames=2000 --set stop.scene=12 --set stop.timer=300
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60 RESULT_VARIABLE range_result
    OUTPUT_VARIABLE range_output ERROR_VARIABLE range_error)
if(NOT range_result EQUAL 0 OR
   NOT range_error MATCHES "racetrace [^\n]* timer=240 accelerator=256" OR
   NOT range_error MATCHES "racetrace [^\n]* timer=260 accelerator=0")
    message(FATAL_ERROR "Scene-relative held input did not press/release: ${range_output}${range_error}")
endif()
