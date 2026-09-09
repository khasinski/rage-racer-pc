string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/scene-capture-${id}")
file(MAKE_DIRECTORY "${root}")
function(capture name)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
        RAGE_PORT_SMOKE_FRAMES=1250 RAGE_PORT_RAW_INPUT_SCRIPT=400:START,500:START,650:CROSS
        "RAGE_PORT_SCENE_TRACE=${root}/${name}.log" "${GAME}"
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 360 RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Scene capture ${name} failed: ${root}\n${output}${error}")
    endif()
endfunction()
capture(first)
capture(second)
execute_process(COMMAND "${CHECK}" "${root}/first.log" "${root}/second.log"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Scene capture check failed: ${root}\n${output}${error}")
endif()
