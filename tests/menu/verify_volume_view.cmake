set(disc "$ENV{RAGE_PORT_DISC_CUE}")
if(NOT EXISTS "${disc}")
    message("SKIP: volume view needs RAGE_PORT_DISC_CUE")
    return()
endif()
set(root "${OUTPUT_ROOT}/volume-view")
file(MAKE_DIRECTORY "${root}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    "${GAME}" --set "disc.image=${disc}" --set race.enabled=false
    --set video.internal_scale=1 --set run.frames=1500
    --set input.state_script=4@40:UP,4@44:UP,4@48:CONFIRM,23@65:DOWN,23@85:DOWN,23@105:CONFIRM
    --set "capture.directory=${root}" --set capture.scene=23
    --set capture.timer_min=150 --set capture.timer_max=150 --set capture.timer_stride=1
    --set "diagnostics.modern_dump=${root}/modern.ppm"
    --set diagnostics.modern_dump_offscreen=true
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 120 RESULT_VARIABLE result
    OUTPUT_FILE "${root}/game.log" ERROR_FILE "${root}/game.log")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Volume menu run failed; see ${root}/game.log")
endif()
execute_process(COMMAND "${CHECK}" volume "${root}/timer-00150-s23.ppm"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Volume menu labels/bars are missing; see ${root}")
endif()
