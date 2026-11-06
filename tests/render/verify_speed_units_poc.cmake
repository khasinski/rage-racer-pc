# Opt-in retail disc integration check. Never stores game artwork in the repo.
# cmake -DGAME=... -DSOURCE=... -DOUTPUT_ROOT=... [-DHD_MOD=...] -P this-file
set(root "${OUTPUT_ROOT}/speed-units-all")
if(NOT DEFINED DISC)
    set(DISC "${SOURCE}/disc/PAL/Rage Racer (Europe).cue")
endif()
if(NOT DEFINED OVERRIDE)
    set(OVERRIDE mph)
endif()
if(NOT DEFINED RENDERER)
    set(RENDERER modern)
endif()
if(NOT DEFINED CARS)
    set(CARS 0 1 2 3 4 5 6 7 8 9 10 11 12)
endif()
if(NOT DEFINED RACE_MODE)
    set(RACE_MODE time-attack)
endif()
if(NOT DEFINED RACE_CLASS)
    set(RACE_CLASS 0)
endif()
file(MAKE_DIRECTORY "${root}")
foreach(car IN LISTS CARS)
    foreach(units IN ITEMS auto "${OVERRIDE}")
        set(name "car-${car}-${units}")
        set(mod "${root}/no-mod")
        if(car EQUAL 3 AND DEFINED HD_MOD)
            set(mod "${HD_MOD}")
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E env
            PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
            "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
            --set "disc.image=${DISC}"
            --set "race.mode=${RACE_MODE}" --set "race.car=${car}"
            --set "race.class=${RACE_CLASS}" --set race.course=0 --set start.random_seed=1
            --set start.freeze=false --set input.disable_host=true
            --set input.state_script=12@177-450:CROSS
            --set run.frames=1600 --set stop.scene=12 --set stop.timer=440
            --set video.fps=logic --set "video.renderer=${RENDERER}"
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
        if("${output}${error}" MATCHES "unit override rejected")
            message(FATAL_ERROR "Retail unit override rejected: ${name}")
        endif()
        if(DEFINED DIFF_TOOL AND DEFINED SOURCE_REFERENCE_ROOT AND DEFINED TARGET_REFERENCE_ROOT)
            set(referenceRoot "${TARGET_REFERENCE_ROOT}")
            if(units STREQUAL auto)
                set(referenceRoot "${SOURCE_REFERENCE_ROOT}")
            endif()
            execute_process(COMMAND "${DIFF_TOOL}" "${car}"
                "${referenceRoot}/car-${car}.vram" "${root}/${name}.vram"
                "${root}/${name}-reference-check.rrhud"
                RESULT_VARIABLE checkResult OUTPUT_VARIABLE checked)
            if(NOT checkResult EQUAL 0 OR NOT checked MATCHES "changed_pixels=0.*palette_bytes=0")
                message(FATAL_ERROR "Runtime HUD differs from retail reference: ${name}: ${checked}")
            endif()
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
message(STATUS "Selected car simulations identical between units: ${root}")
