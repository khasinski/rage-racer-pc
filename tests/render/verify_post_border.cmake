string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/post-border-${id}")
file(MAKE_DIRECTORY "${root}/mods")
execute_process(COMMAND "${FIXTURE}" write-atlas "${root}/mods/facade.png" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Facade fixture failed")
endif()
file(WRITE "${root}/mods/mod.toml" "[textures]\n\"track.mid1.terrain.material.211.variant.2\" = \"facade.png\"\n")
foreach(hd RANGE 0 1)
    set(mod "")
    if(hd)
        set(mod "${root}/mods")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1
        SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1 "${GAME}"
        --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
        --set modern.assets=disc --set modern.authored_cars=0 --set "hd.store=${root}/hd"
        --set "mods.directory=${mod}" --set mods.random_time_of_day=false
        --set video.renderer=modern --set video.internal_scale=4 --set video.aspect=16:9
        --set video.fps=logic --set video.post=fxaa --set video.grading=off
        --set race.car=3 --set race.course=1 --set race.class=0
        --set start.player_track_point=292 --set start.player_x=35838
        --set start.player_z=29629 --set start.player_heading=3978
        --set start.camera=0 --set start.freeze=true --set sync.random=11@0=12345
        --set input.disable_host=true --set run.frames=1800 --set stop.scene=12 --set stop.timer=400
        --set "diagnostics.modern_dump=${root}/${hd}.ppm"
        --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=380
        --set diagnostics.modern_dump_offscreen=true
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 25 RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Post border scenario ${hd} failed: ${root}\n${output}${error}")
    endif()
endforeach()
execute_process(COMMAND "${CHECK}" post-border "${root}/0.ppm" "${root}/1.ppm"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Post filtering leaked across screen borders: ${root}\n${output}${error}")
endif()
