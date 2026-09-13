string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/prologue-geometry-${id}")
set(capture "${root}/presented.ppm")
file(MAKE_DIRECTORY "${root}")

execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=900
    RAGE_PORT_SMOKE_STOP_SCENE=32 RAGE_PORT_SMOKE_STOP_SCENE_TIMER=180
    RAGE_PORT_INPUT_SCRIPT=400:START,500:START
    "${GAME}" --set video.internal_scale=1 --set video.aspect=4:3
    --set video.post=none --set "diagnostics.modern_dump=${capture}"
    --set diagnostics.modern_dump_scene_id=32
    --set diagnostics.modern_dump_timer=175
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
file(WRITE "${root}/game.log" "${log}")
if(NOT result EQUAL 0 OR NOT log MATCHES "stopped at frame 878, scene 32")
    message(FATAL_ERROR "Prologue scenario failed: ${root}\n${log}")
endif()
execute_process(COMMAND "${CHECK}" prologue "${capture}"
    RESULT_VARIABLE check_result OUTPUT_VARIABLE check_output
    ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR
        "Modern Grand Prix prologue contains no course geometry: ${root}\n${check_output}${check_error}")
endif()
message(STATUS "Grand Prix prologue presents cars and course geometry: ${root}")
