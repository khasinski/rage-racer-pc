# Opt-in PAL disc integration check. Never stores game artwork in the repo.
# cmake -DGAME=... -DSOURCE=... -DOUTPUT_ROOT=... [-DHD_MOD=...] -P this-file
set(root "${OUTPUT_ROOT}/speed-units-all")
file(MAKE_DIRECTORY "${root}")
foreach(car RANGE 0 12)
    foreach(units IN ITEMS auto mph)
        set(name "car-${car}-${units}")
        set(mod "${root}/no-mod")
        if(car EQUAL 3 AND DEFINED HD_MOD)
            set(mod "${HD_MOD}")
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E env
            PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
            "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
            --set "disc.image=${SOURCE}/disc/PAL/Rage Racer (Europe).cue"
            --set race.mode=time-attack --set "race.car=${car}"
            --set race.class=0 --set race.course=0 --set start.random_seed=1
            --set start.freeze=false --set input.disable_host=true
            --set input.state_script=12@177-450:CROSS
            --set run.frames=1600 --set stop.scene=12 --set stop.timer=440
            --set video.fps=logic --set video.renderer=modern
            --set video.internal_scale=3 --set video.aspect=4:3
            --set memory.profile=normal --set mods.random_time_of_day=false
            --set "mods.directory=${mod}" --set "hd.store=${root}/no-pack"
            --set "poc.speed_units=${units}" --set report.hud_layout=true
            --set "dump.vram=${root}/${name}.vram"
            --set "diagnostics.modern_dump=${root}/${name}.ppm"
            --set diagnostics.modern_dump_scene_id=12
            --set diagnostics.modern_dump_timer=430
            --set diagnostics.modern_dump_offscreen=true
            WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60
            RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
        file(WRITE "${root}/${name}.log" "${output}${error}")
        if(NOT result EQUAL 0 OR NOT EXISTS "${root}/${name}.ppm")
            message(FATAL_ERROR "Speed-units capture failed: ${name}: ${result}")
        endif()
        string(REGEX MATCH "Rage Racer smoke stopped[^\n]+" state "${output}")
        if(NOT state)
            message(FATAL_ERROR "Missing final simulation diagnostics: ${name}")
        endif()
        if(units STREQUAL auto)
            set(reference "${state}")
        elseif(NOT reference STREQUAL state)
            message(FATAL_ERROR "Units changed simulation: car ${car}")
        endif()
        message(STATUS "Captured ${name}")
    endforeach()
endforeach()
message(STATUS "All thirteen PAL car simulations identical between units: ${root}")
