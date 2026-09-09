string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/trophy-${id}")
file(MAKE_DIRECTORY "${root}")
set(capture "${root}/trophy.ppm")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_INPUT_SCRIPT=400:START,500:UP,520:CROSS "${GAME}"
    --set run.frames=2200 --set hooks.option_sweep=true --set stop.scene=23
    --set stop.timer=350 --set "capture.path=${capture}"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 135 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Trophy View run failed: ${root}\n${output}${error}")
endif()
execute_process(COMMAND "${CHECK}" trophy "${capture}" RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "Trophy View image check failed: ${root}\n${check_output}${check_error}")
endif()
