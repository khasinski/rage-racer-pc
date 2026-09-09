string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/prologue-${STYLE}-${id}")
file(MAKE_DIRECTORY "${root}")
set(capture "${root}/prologue.ppm")
set(style_args)
if(STYLE STREQUAL "japanese")
    list(APPEND style_args --set content.prologue=japanese)
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=1250 RAGE_PORT_RAW_INPUT_SCRIPT=400:START,500:START,650:CROSS
    "RAGE_PORT_CAPTURE_PATH=${capture}" "${GAME}" ${style_args}
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 75 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result EQUAL 0 OR log MATCHES "primitive buffer exhausted")
    message(FATAL_ERROR "${STYLE} prologue run failed: ${root}\n${log}")
endif()
if(STYLE STREQUAL "japanese" AND NOT log MATCHES "using the extended Japanese-release prologue text")
    message(FATAL_ERROR "Japanese prologue option was not applied: ${root}")
endif()
execute_process(COMMAND "${CHECK}" prologue "${capture}" RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "${STYLE} prologue image check failed: ${root}\n${check_output}${check_error}")
endif()
