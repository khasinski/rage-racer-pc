string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/native-waterfall-${id}")
file(MAKE_DIRECTORY "${root}")
set(asset_options)
if(DEFINED DISC_IMAGE)
    list(APPEND asset_options --set "disc.image=${DISC_IMAGE}")
endif()
if(DEFINED HD_STORE)
    list(APPEND asset_options --set "hd.store=${HD_STORE}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1
    SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1
    "${GAME}" --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
    ${asset_options}
    --set modern.assets=disc --set modern.authored_cars=0 --set mods.directory=
    --set mods.random_time_of_day=false
    --set video.renderer=modern --set video.internal_scale=4 --set video.aspect=16:9
    --set video.fps=logic --set race.car=3 --set race.course=2 --set race.class=0
    --set run.frames=1800 --set stop.scene=12 --set stop.timer=450
    --set start.player_track_point=225 --set start.freeze=true --set start.camera=1
    --set sync.random=11@0=12345 --set input.disable_host=true
    --set "diagnostics.modern_dump=${root}/waterfall.ppm"
    --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=430
    --set diagnostics.modern_dump_every=1
    --set diagnostics.modern_dump_offscreen=true
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 50 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(WRITE "${root}/stdout.txt" "${output}")
file(WRITE "${root}/stderr.txt" "${error}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Native waterfall scenario failed: ${root}\n${output}${error}")
endif()
file(GLOB captures "${root}/waterfall.ppm-*.ppm")
list(LENGTH captures capture_count)
if(capture_count LESS 10)
    message(FATAL_ERROR "Insufficient waterfall animation coverage: ${capture_count} frames in ${root}")
endif()
foreach(capture IN LISTS captures)
    execute_process(COMMAND "${CHECK}" waterfall-native "${capture}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Native waterfall is clipped into the cliff: ${capture}\n${output}${error}")
    endif()
endforeach()
message(STATUS "Waterfall surface remains visible across ${capture_count} consecutive animation frames: ${root}")
