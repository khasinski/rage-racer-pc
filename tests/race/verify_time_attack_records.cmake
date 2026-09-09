if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        SDL_AUDIODRIVER=dummy
        RAGE_PORT_SMOKE_FRAMES=3250
        RAGE_PORT_SMOKE_FINISH_FRAME=2000
        RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME=3000
        RAGE_PORT_INPUT_SCRIPT=950:CROSS,1100:CROSS,1200:CROSS,1264-2600:CROSS
        RAGE_PORT_STATE_INPUT_SCRIPT=4@80@0:START,4@180@2:DOWN,4@220@2:CROSS
        "${GAME}"
    WORKING_DIRECTORY "${SOURCE}"
    TIMEOUT 210
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Time Attack scenario failed (${result}):\n${log}")
endif()
foreach(transition "scene=17 frontend=3" "scene=20 frontend=3"
                   "scene=21 frontend=3")
    string(FIND "${log}" "${transition}" position)
    if(position LESS 0)
        message(FATAL_ERROR "Time Attack missed transition ${transition}:\n${log}")
    endif()
endforeach()
foreach(failure "primitive buffer exhausted" "ERROR: AddressSanitizer")
    string(FIND "${log}" "${failure}" position)
    if(NOT position LESS 0)
        message(FATAL_ERROR "record path reported ${failure}:\n${log}")
    endif()
endforeach()
message(STATUS "Time Attack rendered results and record-name entry")
