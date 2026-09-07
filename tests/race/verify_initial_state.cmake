execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    SDL_AUDIODRIVER=dummy RAGE_PORT_SMOKE_FRAMES=1 RAGE_PORT_SMOKE_INITIAL_STATE=1
    "${GAME}" WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Initial-state run failed (${result}):\n${log}")
endif()
set(expected "initial state: time_attack_enabled=1 selected_car=3")
string(FIND "${log}" "${expected}" position)
if(position LESS 0)
    message(FATAL_ERROR "Missing '${expected}':\n${log}")
endif()
message(STATUS "${expected}")
