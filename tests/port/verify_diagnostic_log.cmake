# Exercise the production diagnostic sink through the existing smoke hook.
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef suffix)
set(directory "${OUTPUT_ROOT}/diagnostic-log-${suffix}")
file(MAKE_DIRECTORY "${directory}")
set(log_path "${directory}/runtime.log")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    SDL_AUDIODRIVER=dummy RAGE_PORT_TEST_LOG=1
    "RAGE_PORT_LOG_PATH=${log_path}" RAGE_PORT_SMOKE_FRAMES=2
    "${GAME}"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 45
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(WRITE "${directory}/process.log" "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Diagnostic log run failed (${result}): ${directory}")
endif()
if(NOT EXISTS "${log_path}")
    message(FATAL_ERROR "Diagnostic log was not created: ${directory}")
endif()
file(READ "${log_path}" log)
foreach(expected "=== Rage Racer session" "smoke state frame=1")
    string(FIND "${log}" "${expected}" position)
    if(position LESS 0)
        message(FATAL_ERROR "Diagnostic log missed '${expected}': ${directory}")
    endif()
endforeach()
message(STATUS "Persistent diagnostic log verified: ${directory}")
