string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/sky-atlas-${id}")
file(MAKE_DIRECTORY "${root}/mods")
execute_process(COMMAND "${FIXTURE}" write-sky "${root}/mods/clouds.png" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Sky atlas fixture failed")
endif()
file(WRITE "${root}/mods/mod.toml" "[textures]\n\"track.big1.sky.atlas\" = \"clouds.png\"\n\"track.big2.sky.atlas\" = \"clouds.png\"\n")
# Different authored headings/bank angles and environment tile rows. The last
# sample also recreates presentation resources with the HD atlas installed.
foreach(sample RANGE 0 2)
    if(sample EQUAL 0)
        set(point 0)
        set(class 0)
    elseif(sample EQUAL 1)
        set(point 120)
        set(class 0)
    else()
        set(point 120)
        set(class 1)
    endif()
    foreach(hd RANGE 0 1)
        set(mod "")
        if(hd)
            set(mod "${root}/mods")
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1
            SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1 "${GAME}"
            --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
            --set modern.assets=disc --set modern.authored_cars=0 --set "mods.directory=${mod}"
            --set video.renderer=modern --set video.internal_scale=2 --set video.aspect=16:9
            --set video.fps=logic --set video.post=none --set video.grading=off
            --set race.car=3 --set race.course=0 --set "race.class=${class}"
            --set "start.player_track_point=${point}" --set start.freeze=true
            --set sync.random=11@0=12345 --set input.disable_host=true
            --set run.frames=1800 --set stop.scene=12 --set stop.timer=400
            --set "diagnostics.modern_dump=${root}/${sample}-${hd}.ppm"
            --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=380
            --set diagnostics.modern_dump_offscreen=true
            --set "diagnostics.renderer_restart_count=${sample}"
            --set diagnostics.renderer_restart_frame=100
            WORKING_DIRECTORY "${SOURCE}" TIMEOUT 25 RESULT_VARIABLE result
            OUTPUT_VARIABLE output ERROR_VARIABLE error)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Sky scenario ${sample}/${hd} failed: ${root}\n${output}${error}")
        endif()
    endforeach()
    execute_process(COMMAND "${CHECK}" sky-atlas "${root}/${sample}-0.ppm" "${root}/${sample}-1.ppm"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Sky override missing or affected the world: ${root}\n${output}${error}")
    endif()
endforeach()
