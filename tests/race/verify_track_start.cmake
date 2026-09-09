if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env SDL_AUDIODRIVER=dummy
        "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
        --set run.frames=2600
        --set stop.scene=12
        --set stop.timer=1
        --set race.grid=0,1,2,3,4,5,6,7,8,9,10
        --set start.player_track_point=120
        --set start.rival_track_points=118,116,114,112
    WORKING_DIRECTORY "${SOURCE}"
    TIMEOUT 150
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "track-start scenario failed (${result}):\n${log}")
endif()
foreach(required
    "scenario-start player point=120 .*progress=[1-9][0-9]* section=[0-9]+"
    "scenario-start rival=0 point=118 .*progress=[1-9][0-9]* section=[0-9]+"
    "scenario-start rival=2 point=114 .*progress=[1-9][0-9]* section=[0-9]+"
    "custom track start applied player=120 rivals=4 points=[0-9]+"
    "smoke synchronized stop")
    if(NOT log MATCHES "${required}")
        message(FATAL_ERROR "track start missed ${required}:\n${log}")
    endif()
endforeach()
if(log MATCHES "ERROR:" OR log MATCHES "runtime error:")
    message(FATAL_ERROR "track start emitted a runtime error:\n${log}")
endif()
message(STATUS "scenario track-point starts produce valid player and rival state")
