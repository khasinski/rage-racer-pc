string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/grand-prix-${id}")
file(MAKE_DIRECTORY "${root}")
set(capture "${root}/grand-prix.ppm")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=1500 RAGE_PORT_SMOKE_STOP_SCENE=12 RAGE_PORT_SMOKE_STOP_SCENE_TIMER=56
    RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,950:CROSS,1100:CROSS,1200:CROSS
    "RAGE_PORT_CAPTURE_PATH=${capture}" RAGE_PORT_SMOKE_CAMERA_STATE=1 RAGE_PORT_TERRAIN_TRACE_TIMER=56 "${GAME}"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 105 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
foreach(needle "stopped at frame 1420, scene 12" "smoke synchronized stop frame=1420 scene=12 timer=56" "ref_lap=100765" "time_text=1'40\"765")
    if(NOT log MATCHES "${needle}")
        message(FATAL_ERROR "Grand Prix assertion missing ${needle}: ${root}\n${log}")
    endif()
endforeach()
if(NOT result EQUAL 0 OR log MATCHES "primitive buffer exhausted|unsupported command" OR NOT log MATCHES "terrain-lod timer=56.*mirror=0.*shift=10")
    message(FATAL_ERROR "Grand Prix scenario failed: ${root}\n${log}")
endif()
execute_process(COMMAND "${CHECK}" grand_prix "${capture}" RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output ERROR_VARIABLE check_error)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "Grand Prix image check failed: ${root}\n${check_output}${check_error}")
endif()
