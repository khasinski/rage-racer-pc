set(disc "$ENV{RAGE_PORT_DISC_CUE}")
if(NOT EXISTS "${disc}")
    set(disc "${SOURCE}/disc/PAL/Rage Racer (Europe).cue")
endif()
if(NOT EXISTS "${disc}")
    message("SKIP: provide RAGE_PORT_DISC_CUE for the PAL terrain regression")
    return()
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/terrain-seams-${id}")
file(MAKE_DIRECTORY "${root}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
    "${GAME}" --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
    --set "disc.image=${disc}" --set modern.assets=disc --set video.renderer=modern
    --set video.internal_scale=3 --set video.aspect=16:9 --set video.fps=logic
    --set video.post=none --set video.grading=off --set race.course=0 --set race.class=0 --set race.car=3
    --set start.player_track_point=0 --set start.freeze=true --set mods.random_time_of_day=false
    --set input.disable_host=true --set run.frames=1800 --set stop.scene=12 --set stop.timer=400
    --set "diagnostics.modern_dump=${root}/road.ppm" --set diagnostics.modern_dump_scene_id=12
    --set diagnostics.modern_dump_timer=380 --set diagnostics.modern_dump_offscreen=true
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 45 RESULT_VARIABLE result
    OUTPUT_FILE "${root}/stdout.log" ERROR_FILE "${root}/stderr.log")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Terrain seam capture failed: ${root}")
endif()
execute_process(COMMAND "${CHECK}" road-seam "${root}/road.ppm"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Open terrain seam: ${root}\n${output}${error}")
endif()
message(STATUS "${output}")
