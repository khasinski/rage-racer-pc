string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/pegase-${id}")
file(MAKE_DIRECTORY "${root}")
set(native_assets "${root}/native-assets")
file(MAKE_DIRECTORY "${native_assets}")
set(capture "${root}/pegase.ppm")
execute_process(COMMAND "${FIXTURE}" "${native_assets}" RESULT_VARIABLE fixture_result)
if(NOT fixture_result EQUAL 0)
    message(FATAL_ERROR "Cannot generate native vehicle fixture: ${root}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    "RAGE_PORT_MODERN_ASSETS=${native_assets}" RAGE_PORT_MODERN_ASSET_TRACE=1
    "${GAME}" --scenario "${SOURCE}/race-scenario.ini" --set run.frames=3000
    --set stop.scene=12 --set stop.timer=431 --set race.car=2
    --set race.grid=2,1,0,3,4,5,6,7,8,9,10 --set start.player_track_point=0
    --set start.rival_track_points=2,4,6 --set start.freeze=true --set start.camera=1
    --set video.renderer=modern --set video.internal_scale=1 --set video.aspect=16:9
    --set "diagnostics.modern_dump=${capture}" --set diagnostics.modern_dump_frame=620
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 165 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result EQUAL 0 OR NOT EXISTS "${capture}" OR NOT log MATCHES "course=0 car=2" OR
   NOT log MATCHES "native material asset=24 set=0")
    message(FATAL_ERROR "Age Pegase capture failed: ${root}\n${log}")
endif()
execute_process(COMMAND "${CHECK}" pegase "${capture}" RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "Age Pegase image check failed: ${root}\n${check_output}${check_error}")
endif()
