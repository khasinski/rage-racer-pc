cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED GAME OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "GAME, SOURCE and OUTPUT are required")
endif()
if(NOT DEFINED DISC)
    set(DISC "$ENV{RAGE_PORT_DISC_CUE}")
endif()
if(NOT EXISTS "${DISC}")
    message("SKIP: provide DISC or RAGE_PORT_DISC_CUE")
    return()
endif()
# Exercise real bank transitions: title -> attract -> interrupted title ->
# Grand Prix prologue -> first race. Compare resident GPU geometry with the
# uncached CPU expansion of the same meshes, including the prologue.
foreach(mode resident reference)
    set(directory "${OUTPUT}/${mode}")
    file(MAKE_DIRECTORY "${directory}")
    file(GLOB stale "${directory}/frame-*.ppm")
    if(stale)
        file(REMOVE ${stale})
    endif()
    set(cpu false)
    if(mode STREQUAL "reference")
        set(cpu true)
    endif()
    execute_process(COMMAND "${GAME}" --config "${SOURCE}/rage-port.ini"
        --set "disc.image=${DISC}" --set race.enabled=false
        --set run.frames=3000 --set stop.scene=12 --set stop.timer=110
        --set input.disable_host=true
        --set input.script=1450:START,1550:START,1700:CROSS,2000:CROSS,2150:CROSS,2250:CROSS
        --set video.renderer=modern --set video.fps=logic
        --set video.internal_scale=1 --set video.post=none
        --set "diagnostics.modern_cpu_geometry=${cpu}"
        --set diagnostics.scene_trace=true
        --set "diagnostics.modern_dump=${directory}/frame"
        --set diagnostics.modern_dump_frame=1850
        --set diagnostics.modern_dump_every=100
        --set diagnostics.modern_dump_offscreen=true
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE result
        OUTPUT_FILE "${directory}/stdout.log"
        ERROR_FILE "${directory}/stderr.log" TIMEOUT 150)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${mode} attract/GP run failed: ${result}")
    endif()
    file(READ "${directory}/stderr.log" trace)
    foreach(scene 30 32 12)
        if(NOT trace MATCHES "smoke state frame=[0-9]+ scene=${scene} ")
            message(FATAL_ERROR "${mode}: scene ${scene} not reached")
        endif()
    endforeach()
    if(NOT trace MATCHES "smoke synchronized stop frame=[0-9]+ scene=12 timer=110")
        message(FATAL_ERROR "${mode}: GP race did not complete test interval")
    endif()
endforeach()
file(GLOB images RELATIVE "${OUTPUT}/reference" "${OUTPUT}/reference/frame-*.ppm")
list(LENGTH images count)
if(count LESS 4)
    message(FATAL_ERROR "Too few prologue/race GPU captures: ${count}")
endif()
foreach(image IN LISTS images)
    file(SHA256 "${OUTPUT}/reference/${image}" reference)
    file(SHA256 "${OUTPUT}/resident/${image}" resident)
    if(NOT resident STREQUAL reference)
        message(FATAL_ERROR "Stale resident geometry after attract: ${image}")
    endif()
endforeach()
message(STATUS "Attract -> GP: ${count} GPU frames equal the reference")
