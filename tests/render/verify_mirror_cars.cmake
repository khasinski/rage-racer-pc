string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/mirror-cars-${id}")
file(MAKE_DIRECTORY "${root}/native-assets")
execute_process(COMMAND "${FIXTURE}" "${root}/native-assets" RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Cannot generate fixture: ${root}")
endif()
foreach(renderer classic modern)
    set(out "${root}/${renderer}")
    file(MAKE_DIRECTORY "${out}")
    set(env SDL_AUDIODRIVER=dummy)
    set(extra)
    if(renderer STREQUAL "modern")
        list(APPEND env "RAGE_PORT_MODERN_ASSETS=${root}/native-assets" RAGE_PORT_MODERN_ASSET_TRACE=1)
        set(extra --set video.internal_scale=1 --set video.aspect=4:3
            --set "diagnostics.modern_dump=${out}/native-mirror.ppm" --set diagnostics.modern_dump_frame=620)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${env} "${GAME}"
        --scenario "${SOURCE}/race-scenario.ini" --set run.frames=3000 --set stop.scene=12
        --set stop.timer=431 --set race.grid=0,1,2,3,4,5,6,7,8,9,10
        --set start.player_track_point=50 --set start.rival_track_points=52,54,56
        --set start.freeze=true --set "capture.directory=${out}" --set capture.scene=12
        --set capture.timer_min=430 --set capture.timer_max=430 --set capture.timer_stride=1
        --set "video.renderer=${renderer}" ${extra} WORKING_DIRECTORY "${SOURCE}"
        TIMEOUT 165 RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    file(WRITE "${out}/game.log" "${log}")
    if(NOT result STREQUAL "0" OR NOT EXISTS "${out}/timer-00430-s12.ppm")
        message(FATAL_ERROR "Missing ${renderer} capture: ${root}")
    endif()
endforeach()
if(NOT log MATCHES "mirror_vertices=[1-9][0-9]* mirror_spans=[1-9][0-9]* mirror_vehicle_spans=[1-9][0-9]*" OR
   NOT log MATCHES "native draws frame=[0-9]+ draws=[1-9][0-9]* vertices=[1-9][0-9]* view=mirror")
    message(FATAL_ERROR "Native rear scene not built/submitted: ${root}")
endif()
execute_process(COMMAND "${CHECK}" "${root}/classic/timer-00430-s12.ppm"
    "${root}/modern/native-mirror.ppm" RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Mirror pixels failed: ${root}")
endif()
message(STATUS "Compiled mirror cars passed: ${root}")
