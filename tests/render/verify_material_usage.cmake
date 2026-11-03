cmake_minimum_required(VERSION 3.20)
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/material-usage-${id}")
file(MAKE_DIRECTORY "${root}")
foreach(camera RANGE 0 1)
    foreach(trace RANGE 0 1)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1
            SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1 "${GAME}"
            --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
            --set modern.assets=disc --set modern.authored_cars=0
            --set mods.directory= --set "hd.store=${root}/hd"
            --set video.renderer=modern --set video.internal_scale=2 --set video.aspect=16:9
            --set video.fps=logic --set mods.random_time_of_day=false
            --set race.car=3 --set race.course=1 --set race.class=0
            --set start.player_track_point=318 --set "start.camera=${camera}"
            --set start.freeze=true --set input.disable_host=true
            --set run.frames=1800 --set stop.scene=12 --set stop.timer=400
            --set diagnostics.test_log=true --set diagnostics.performance_trace=true
            --set "diagnostics.material_usage=${trace}"
            --set diagnostics.material_usage_start_frame=350
            --set "diagnostics.log=${root}/${camera}-${trace}.log"
            --set "diagnostics.modern_dump=${root}/${camera}-${trace}.ppm"
            --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=380
            --set diagnostics.modern_dump_offscreen=true
            WORKING_DIRECTORY "${SOURCE}" TIMEOUT 25 RESULT_VARIABLE result
            OUTPUT_VARIABLE output ERROR_VARIABLE error)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Material usage scenario failed: ${root}\n${output}${error}")
        endif()
        file(READ "${root}/${camera}-${trace}.log" log)
        if(trace)
            if(NOT log MATCHES "modern-texture asset=90 set=2 material=76 variant=0")
                message(FATAL_ERROR "Expected preloaded alternate material missing: ${root}")
            endif()
            string(REGEX MATCHALL "native-material-use: [^\n]+" records "${log}")
            if(NOT records)
                message(FATAL_ERROR "No submitted-material records: ${root}")
            endif()
            set(terrainKeys "")
            set(mainColumn FALSE)
            set(mirror FALSE)
            foreach(record IN LISTS records)
                string(REPLACE "native-material-use: " "" json "${record}")
                foreach(field frame view asset set material variant vertices)
                    string(JSON ${field} GET "${json}" ${field})
                endforeach()
                if(frame LESS 350 OR vertices LESS 1)
                    message(FATAL_ERROR "Usage record precedes cutoff or has no geometry: ${json}")
                endif()
                if("${view}" STREQUAL "mirror")
                    set(mirror TRUE)
                endif()
                if(set EQUAL 2)
                    set(key "${view}/${asset}/${material}/${variant}")
                    if(key IN_LIST terrainKeys)
                        message(FATAL_ERROR "Terrain usage not deduplicated: ${key}")
                    endif()
                    list(APPEND terrainKeys "${key}")
                endif()
                if("${view}" STREQUAL "main" AND asset EQUAL 90 AND set EQUAL 2 AND material EQUAL 76)
                    if(variant EQUAL 0)
                        message(FATAL_ERROR "Cache-only alternate reported as a submitted draw: ${json}")
                    elseif(variant EQUAL 2)
                        set(mainColumn TRUE)
                    endif()
                endif()
            endforeach()
            if(NOT mainColumn)
                message(FATAL_ERROR "Submitted main-view column missing: ${root}")
            endif()
            if(camera EQUAL 0 AND NOT mirror)
                message(FATAL_ERROR "Mirror submissions missing: ${root}")
            endif()
        elseif(log MATCHES "native-material-use:")
            message(FATAL_ERROR "Material usage tracing is not opt-in: ${root}")
        endif()
    endforeach()
    file(SHA256 "${root}/${camera}-0.ppm" before)
    file(SHA256 "${root}/${camera}-1.ppm" after)
    if(NOT before STREQUAL after)
        message(FATAL_ERROR "Usage tracing changed rendered pixels: ${root}")
    endif()
endforeach()
