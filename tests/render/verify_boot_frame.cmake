string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/boot-${id}")
file(MAKE_DIRECTORY "${root}")
set(capture "${root}/boot.ppm")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=20 "RAGE_PORT_CAPTURE_PATH=${capture}" "${GAME}"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 45 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Boot capture failed: ${root}\n${output}${error}")
endif()
execute_process(COMMAND "${CHECK}" boot "${capture}" RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "Boot image check failed: ${root}\n${check_output}${check_error}")
endif()
