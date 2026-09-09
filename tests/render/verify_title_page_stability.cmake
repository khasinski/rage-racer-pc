string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/title-pages-${id}")
file(MAKE_DIRECTORY "${root}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=499 "RAGE_PORT_SMOKE_CAPTURE_DIR=${root}"
    RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE=1 RAGE_PORT_SMOKE_CAPTURE_SCENE=4
    RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN=180 RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX=181
    RAGE_PORT_SMOKE_CAPTURE_ALL_PHASES=1 "${GAME}"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(GLOB captures "${root}/timer-*.ppm")
list(LENGTH captures count)
if(NOT result EQUAL 0 OR NOT count EQUAL 2)
    message(FATAL_ERROR "Title capture failed: ${root}\n${output}${error}")
endif()
list(SORT captures)
list(GET captures 0 first)
list(GET captures 1 second)
execute_process(COMMAND "${CHECK}" title "${first}" "${second}" RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "Title artwork check failed: ${root}\n${check_output}${check_error}")
endif()
