string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/waterfall-${id}")
file(MAKE_DIRECTORY "${root}")
set(capture "${root}/waterfall.ppm")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
    --set race.course=2 --set run.frames=3000 --set stop.scene=12
    --set stop.timer=430 --set start.player_track_point=225 --set start.freeze=true
    --set "capture.path=${capture}" --set video.renderer=classic
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 165 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result EQUAL 0 OR NOT log MATCHES "scene=12 timer=430")
    message(FATAL_ERROR "Waterfall scenario failed: ${root}\n${log}")
endif()
execute_process(COMMAND "${CHECK}" waterfall "${capture}" RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "Waterfall capture failed: ${root}\n${check_output}${check_error}")
endif()
