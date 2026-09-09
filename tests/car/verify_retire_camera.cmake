if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        SDL_AUDIODRIVER=dummy
        RAGE_PORT_SCENARIO=1
        RAGE_PORT_SMOKE_FRAMES=2500
        RAGE_PORT_SMOKE_RETIRE=1
        RAGE_PORT_SMOKE_CAMERA_STATE=1
        "${GAME}"
    WORKING_DIRECTORY "${SOURCE}"
    TIMEOUT 150
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "RETIRE scenario failed (${result}):\n${log}")
endif()
if(NOT log MATCHES "scene [0-9]+,.*race_phase=5.*retire_camera=1")
    message(FATAL_ERROR "RETIRE did not enter chase-camera state:\n${log}")
endif()
if(NOT log MATCHES "camera: pos=")
    message(FATAL_ERROR "RETIRE camera state was not observable:\n${log}")
endif()
if(log MATCHES "ERROR:" OR log MATCHES "runtime error:")
    message(FATAL_ERROR "RETIRE emitted a runtime error:\n${log}")
endif()
message(STATUS "RETIRE retains the chase camera")
