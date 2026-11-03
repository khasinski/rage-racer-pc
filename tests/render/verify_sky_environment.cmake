string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/sky-environment-${id}")
file(MAKE_DIRECTORY "${root}/wrong" "${root}/selected")
execute_process(COMMAND "${FIXTURE}" write-sky "${root}/clouds.png" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Sky fixture failed")
endif()
file(COPY "${root}/clouds.png" DESTINATION "${root}/wrong")
file(COPY "${root}/clouds.png" DESTINATION "${root}/selected")
file(WRITE "${root}/wrong/mod.toml" "[textures]\n\"track.big1.sky.atlas\" = \"clouds.png\"\n")
foreach(seed RANGE 0 1)
    if(seed EQUAL 0)
        set(bank big3)
        set(key 104)
    else()
        set(bank big4)
        set(key 112)
    endif()
    file(WRITE "${root}/selected/mod.toml" "[textures]\n\"track.${bank}.sky.atlas\" = \"clouds.png\"\n")
    foreach(layer baseline wrong selected)
        set(mod "")
        if(NOT layer STREQUAL baseline)
            set(mod "${root}/${layer}")
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1
            SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1 "${GAME}"
            --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
            --set modern.assets=disc --set modern.authored_cars=0 --set "mods.directory=${mod}"
            --set "hd.store=${root}/hd" --set video.renderer=modern
            --set video.internal_scale=2 --set video.aspect=16:9 --set video.fps=logic
            --set video.post=none --set video.grading=off
            --set race.car=3 --set race.course=0 --set race.class=0
            --set start.player_track_point=120 --set start.freeze=true
            --set "start.random_seed=${seed}" --set mods.random_time_of_day=true
            --set input.disable_host=true --set run.frames=1800
            --set stop.scene=12 --set stop.timer=400 --set report.hud_layout=true
            --set "diagnostics.modern_dump=${root}/${seed}-${layer}.ppm"
            --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=380
            --set diagnostics.modern_dump_offscreen=true
            WORKING_DIRECTORY "${SOURCE}" TIMEOUT 25 RESULT_VARIABLE result
            OUTPUT_VARIABLE output ERROR_VARIABLE error)
        if(NOT result EQUAL 0 OR NOT output MATCHES "sky_override_asset_key\":${key}")
            message(FATAL_ERROR "Selected sky profile ${seed}/${layer} failed: ${root}\n${output}${error}")
        endif()
    endforeach()
    file(SHA256 "${root}/${seed}-baseline.ppm" before)
    file(SHA256 "${root}/${seed}-wrong.ppm" wrong)
    if(NOT before STREQUAL wrong)
        message(FATAL_ERROR "Base-class sky incorrectly overrides selected environment: ${root}")
    endif()
    execute_process(COMMAND "${CHECK}" sky-atlas "${root}/${seed}-baseline.ppm" "${root}/${seed}-selected.ppm"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Selected environment sky missing or affected world: ${root}\n${output}${error}")
    endif()
endforeach()
