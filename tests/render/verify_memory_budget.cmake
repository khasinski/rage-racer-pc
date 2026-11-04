string(RANDOM LENGTH 10 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/memory-budget-${id}")
file(MAKE_DIRECTORY "${root}/assets")
execute_process(COMMAND "${FIXTURE}" "${root}/assets" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot prepare memory-budget fixture")
endif()
foreach(case IN ITEMS minimum low pressure)
    set(total 2048)
    set(frame 0)
    set(target "640x360")
    set(profile minimum)
    if(case STREQUAL "low")
        set(total 4096)
        set(target "852x480")
        set(profile low)
    elseif(case STREQUAL "pressure")
        set(total 8192)
        set(frame 100)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
        "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
        --set "modern.assets=${root}/assets" --set video.renderer=modern
        --set memory.profile=auto --set "diagnostics.memory_total_mb=${total}"
        --set diagnostics.memory_available_mb=8192
        --set "diagnostics.memory_pressure_frame=${frame}"
        --set video.internal_scale=4 --set video.aspect=16:9 --set video.fps=logic
        --set race.course=0 --set race.class=0 --set race.car=3
        --set start.freeze=true --set input.disable_host=true
        --set run.frames=1900 --set stop.scene=12 --set stop.timer=500
        --set "diagnostics.modern_dump=${root}/${case}.ppm"
        --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=490
        --set diagnostics.modern_dump_offscreen=true
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 75
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    file(WRITE "${root}/${case}.log" "${log}")
    if(NOT result EQUAL 0 OR NOT EXISTS "${root}/${case}.ppm" OR
       NOT log MATCHES "profile=${profile}" OR
       NOT log MATCHES "modern renderer target ${target}" OR NOT log MATCHES "scene 12")
        message(FATAL_ERROR "Memory-budget ${case} failed: ${root}\n${log}")
    endif()
    if(case STREQUAL "pressure" AND NOT log MATCHES "reason=system low-memory event")
        message(FATAL_ERROR "System memory-pressure event was not handled: ${root}")
    endif()
    if(log MATCHES "renderer switched to classic|low-memory presentation rebuild failed|runtime error:|primitive buffer exhausted")
        message(FATAL_ERROR "Memory-budget ${case} reported a rendering failure: ${root}")
    endif()
endforeach()
message(STATUS "Low-RAM startup and live pressure recovery rendered in modern: ${root}")
