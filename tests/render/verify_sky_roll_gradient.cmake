string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/sky-roll-gradient-${id}")
file(MAKE_DIRECTORY "${root}/mod")
execute_process(COMMAND "${FIXTURE}" write-transparent-sky "${root}/mod/empty.png"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Could not create transparent sky fixture")
endif()
file(WRITE "${root}/mod/mod.toml" "[textures]\n\"track.oval5.sky.atlas\"=\"empty.png\"\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1
    SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1 "${GAME}"
    --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
    --set "disc.image=${SOURCE}/disc/PAL/Rage Racer (Europe).cue"
    --set modern.assets=disc --set modern.authored_cars=0 --set video.renderer=modern
    --set "hd.store=${root}/no-hd" --set "mods.directory=${root}/mod"
    --set mods.random_time_of_day=false --set diagnostics.sky_only=true
    --set video.internal_scale=4 --set video.aspect=16:9 --set video.fps=logic
    --set video.post=none --set race.class=4 --set race.course=3 --set race.car=3
    --set start.player_track_point=64 --set start.camera=1 --set start.freeze=true
    --set input.disable_host=true --set diagnostics.review_camera=false
    --set diagnostics.test_log=true --set "diagnostics.log=${root}/game.log"
    --set sync.random=11@0=12345 --set run.frames=1800 --set stop.scene=12 --set stop.timer=400
    --set "diagnostics.modern_dump=${root}/gradient.ppm"
    --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=380
    --set diagnostics.modern_dump_offscreen=true
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 45 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Rolled gradient capture failed: ${root}\n${output}${error}")
endif()
file(READ "${root}/game.log" log)
if(NOT log MATCHES "native texture override track.oval5.sky.atlas.*empty.png" OR
   NOT log MATCHES "scenario-start player point=64 ")
    message(FATAL_ERROR "Rolled sky probe did not use the requested fixture/start: ${root}")
endif()
execute_process(COMMAND "${CHECK}" sky-roll-gradient "${root}/gradient.ppm"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Sky gradient has a hole: ${root}\n${output}${error}")
endif()
