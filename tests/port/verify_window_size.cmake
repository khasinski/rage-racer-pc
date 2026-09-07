execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    SDL_AUDIODRIVER=dummy RAGE_PORT_SMOKE_FRAMES=1 RAGE_PORT_SMOKE_WINDOW_SIZE=1
    "${GAME}" WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Window-size run failed (${result}):\n${log}")
endif()
string(FIND "${log}" "window size: 640x480" position)
if(position LESS 0)
    message(FATAL_ERROR "Initial window is not logically 640x480:\n${log}")
endif()
message(STATUS "Initial window is logically 640x480")
