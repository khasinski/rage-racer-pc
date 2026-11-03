string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/sky-${id}")
file(MAKE_DIRECTORY "${root}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_DISABLE_HOST_INPUT=1 RAGE_PORT_SYNC_RANDOM=12@0=1
    "RAGE_PORT_SMOKE_CAPTURE_DIR=${root}" RAGE_PORT_SMOKE_CAPTURE_SCENE=12
    RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN=100 RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX=1200
    RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE=50
    "${GAME}" --scenario "${SOURCE}/race-scenario.ini" --set race.class=4
    --set race.course=0 --set run.frames=2600 --set video.renderer=classic
    --set input.state_script=12@177-877:CROSS,12@927-1127:CROSS+LEFT,12@1177-1377:CROSS+RIGHT,12@1427-1527:SQUARE,12@1577-2177:CROSS+LEFT,12@2227-3977:CROSS
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 600 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(GLOB captures "${root}/timer-*.ppm")
list(SORT captures)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Sky run failed: ${root}\n${output}${error}")
endif()
# Input times now refer to scene 12, independent of asset-loading frame counts.
# The 23-frame contact sheet was reviewed after this synchronization change.
# The hard-cut fix reads the previous published world's sky grid instead of
# current->previousCamera, whose history intentionally resets at a shot cut.
execute_process(COMMAND "${CHECK}" sky 0xa47e8618 ${captures} RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "Sky digest failed: ${root}\n${check_output}${check_error}")
endif()
