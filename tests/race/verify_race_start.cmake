if(NOT DEFINED GAME OR NOT DEFINED SOURCE OR NOT DEFINED PPM_CHECK)
    message(FATAL_ERROR "GAME, SOURCE and PPM_CHECK are required")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/race-start-${id}")
file(MAKE_DIRECTORY "${root}")
set(capture "${root}/race.ppm")
set(trace "${root}/spu.csv")
execute_process(COMMAND ${CMAKE_COMMAND} -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=2400 RAGE_PORT_SMOKE_STOP_SCENE=12 RAGE_PORT_SMOKE_STOP_SCENE_TIMER=562
    RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,950:CROSS,1100:CROSS,1200:CROSS,1470-2400:CROSS
    "RAGE_PORT_CAPTURE_PATH=${capture}" RAGE_PORT_TERRAIN_TRACE_TIMER=562 RAGE_PORT_SMOKE_AUDIO_METRICS=1
    "RAGE_PORT_SPU_TRACE=${trace}" "${GAME}" --set video.renderer=classic --set video.aspect=4:3
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 135 RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result EQUAL 0 OR NOT EXISTS "${capture}" OR NOT EXISTS "${trace}")
    message(FATAL_ERROR "Race-start run failed: ${root}\n${log}")
endif()
foreach(required
    "terrain-lod timer=562.*mirror=0.*shift=10" "terrain-lod timer=562.*mirror=1.*shift=9"
    "scene=12 frontend=3 sky_row=0" "scene 12" "speed=[1-9][0-9]* accelerator=256"
    "rpm=[1-9][0-9][0-9][0-9].*terrain_second=[1-9][0-9]*"
    "audio metrics: .*pitch_updates=[1-9][0-9]*"
    "terrain_child_reject=[1-9][0-9]* terrain_child_second=[1-9][0-9]*"
    "model_backface=[1-9][0-9]*")
    if(NOT log MATCHES "${required}")
        message(FATAL_ERROR "Race-start log missed ${required}: ${root}\n${log}")
    endif()
endforeach()
string(REGEX MATCH "loaded=([0-9a-f]+).*scale=[0-9]+ seq_fade=(-?[0-9]+) seq_volume=(-?[0-9]+)" sequence "${log}")
if(NOT sequence)
    message(FATAL_ERROR "Race-start did not report menu-sequence state: ${root}")
endif()
set(sequence_fade "${CMAKE_MATCH_2}")
set(sequence_volume "${CMAKE_MATCH_3}")
math(EXPR loaded "0x${CMAKE_MATCH_1}")
math(EXPR sequence_active "${loaded} & 64")
if(sequence_active OR NOT sequence_fade EQUAL 0 OR NOT sequence_volume EQUAL 0)
    message(FATAL_ERROR "Menu sequence leaked into race: ${sequence}")
endif()
foreach(prefix "14,54272,4216," "16,54272,4216," "17,59128,4216," "18,54272,4216,")
    file(STRINGS "${trace}" trace_rows REGEX "^${prefix}")
    if(NOT trace_rows)
        message(FATAL_ERROR "Race audio never flushed voice ${prefix}: ${root}")
    endif()
endforeach()
foreach(failure "primitive buffer exhausted" "misaligned OT link" "likely corrupted")
    string(FIND "${log}" "${failure}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "Race-start reported ${failure}: ${root}")
    endif()
endforeach()
execute_process(COMMAND "${PPM_CHECK}" race_start "${capture}" RESULT_VARIABLE image_result)
if(NOT image_result EQUAL 0)
    message(FATAL_ERROR "Race-start capture failed: ${root}")
endif()
