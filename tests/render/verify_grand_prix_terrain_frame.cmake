string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/grand-prix-${id}")
file(MAKE_DIRECTORY "${root}")
set(presented "${root}/presented.ppm")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_TEST_DISPLAY_SYNC=1
    RAGE_PORT_SMOKE_FRAMES=1600 RAGE_PORT_SMOKE_STOP_SCENE=12 RAGE_PORT_SMOKE_STOP_SCENE_TIMER=101
    RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,950:CROSS,1100:CROSS,1200:CROSS
    "RAGE_PORT_SMOKE_CAPTURE_DIR=${root}"
    RAGE_PORT_SMOKE_CAPTURE_SCENE=12 RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN=35
    RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX=100 RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE=5
    RAGE_PORT_SMOKE_CAMERA_STATE=1 "RAGE_PORT_MODERN_DUMP=${presented}"
    RAGE_PORT_MODERN_DUMP_FRAME=1399 RAGE_PORT_MODERN_DUMP_SCENE=12
    "${GAME}" --set video.internal_scale=1 --set video.aspect=4:3
    --set video.post=none
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 105 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
foreach(needle "stopped at frame 1465, scene 12" "smoke synchronized stop frame=1465 scene=12 timer=101" "sync=180" "ref_lap=100765")
    if(NOT log MATCHES "${needle}")
        message(FATAL_ERROR "Grand Prix assertion missing ${needle}: ${root}\n${log}")
    endif()
endforeach()
execute_process(COMMAND "${CHECK}" grand_prix "${presented}"
    RESULT_VARIABLE check_result OUTPUT_VARIABLE check_output
    ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR
        "Presented Grand Prix intro contains no race geometry: ${root}\n${check_output}${check_error}")
endif()
if(NOT result EQUAL 0 OR log MATCHES "primitive buffer exhausted|unsupported command")
    message(FATAL_ERROR "Grand Prix scenario failed: ${root}\n${log}")
endif()
foreach(timer 00035 00050 00075 00100)
    set(capture "${root}/timer-${timer}-s12.ppm")
    execute_process(COMMAND "${CHECK}" grand_prix "${capture}"
        RESULT_VARIABLE check_result OUTPUT_VARIABLE check_output
        ERROR_VARIABLE check_error)
    if(NOT check_result EQUAL 0)
        message(FATAL_ERROR
            "Grand Prix intro image check failed at timer ${timer}: ${root}\n${check_output}${check_error}")
    endif()
endforeach()
