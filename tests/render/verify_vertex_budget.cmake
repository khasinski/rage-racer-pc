set(disc "$ENV{RAGE_PORT_DISC_CUE}")
if(NOT EXISTS "${disc}")
    message("SKIP: vertex budget needs RAGE_PORT_DISC_CUE")
    return()
endif()
set(root "${OUTPUT_ROOT}/vertex-budget")
file(MAKE_DIRECTORY "${root}")
foreach(cpu false true)
    file(REMOVE "${root}/${cpu}.ppm")
    # RT-off shaders still require storage-buffer descriptors. This run also
    # regresses the first-scene Vulkan crash caused by leaving them unbound.
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
        "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_vainqure.ini"
        --set "disc.image=${disc}" --set race.class=4 --set race.car=3 --set race.variant=4
        --set start.player_track_point=150 --set start.freeze=true
        --set video.internal_scale=1 --set video.fps=1000
        --set "diagnostics.modern_cpu_geometry=${cpu}" --set diagnostics.performance_trace=true
        --set diagnostics.modern_asset_trace=true
        --set stop.timer=600 --set run.frames=3000
        --set "diagnostics.modern_dump=${root}/${cpu}.ppm"
        --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=550
        --set diagnostics.modern_dump_offscreen=true
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 120 RESULT_VARIABLE result
        OUTPUT_FILE "${root}/${cpu}.log" ERROR_FILE "${root}/${cpu}.log")
    if(NOT result EQUAL 0 OR NOT EXISTS "${root}/${cpu}.ppm")
        message(FATAL_ERROR "Vertex-buffer growth run failed; see ${root}/${cpu}.log")
    endif()
    file(READ "${root}/${cpu}.log" log)
    string(REGEX MATCHALL "native-vertex-grow required=[0-9]+ capacity=[0-9]+ maximum=112000000" growth "${log}")
    if(NOT growth OR NOT log MATCHES "native draws frame=[0-9]+ draws=[1-9][0-9]+")
        message(FATAL_ERROR "No actual buffer growth and rendering observed; see ${root}")
    endif()
    foreach(event IN LISTS growth)
        string(REGEX REPLACE ".*capacity=([0-9]+).*" "\\1" capacity "${event}")
        if(capacity GREATER 16777216)
            message(FATAL_ERROR "Small race frame reserved more than 16 MiB: ${event}")
        endif()
    endforeach()
endforeach()
