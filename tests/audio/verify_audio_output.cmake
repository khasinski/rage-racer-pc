string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/audio-${id}")
file(MAKE_DIRECTORY "${root}")
set(trace "${root}/spu.csv")
set(spu "${root}/spu.bin")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=1400 RAGE_PORT_SMOKE_AUDIO_METRICS=1
    RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,950:CROSS,1100:CROSS,1200:CROSS
    RAGE_PORT_SMOKE_STOP_SCENE=10 RAGE_PORT_SMOKE_STOP_SCENE_TIMER=40
    "RAGE_PORT_SPU_TRACE=${trace}" "RAGE_PORT_DUMP_SPU_RAM=${spu}"
    "${GAME}" --set video.renderer=classic
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 90 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result EQUAL 0 OR NOT EXISTS "${trace}" OR NOT EXISTS "${spu}")
    message(FATAL_ERROR "Audio output run failed: ${root}\n${log}")
endif()
string(REGEX MATCH "audio metrics: frames=([0-9]+) energy=([0-9]+) seq_notes=([0-9]+) seq_voices=([0-9]+) pitch_updates=([0-9]+) cdda=([0-9]+)" metrics "${log}")
if(NOT metrics OR CMAKE_MATCH_1 LESS 10000 OR CMAKE_MATCH_2 LESS 1000000 OR
   CMAKE_MATCH_3 LESS 110 OR CMAKE_MATCH_3 GREATER 140 OR CMAKE_MATCH_4 LESS CMAKE_MATCH_3 OR
   NOT CMAKE_MATCH_6 EQUAL 0)
    message(FATAL_ERROR "Audio metrics invalid: ${root}\n${log}")
endif()
file(READ "${trace}" trace_text)
foreach(row "18,13896,1024,9219,6773,33023,8128" "19,13896,1035,6524,9219,33023,19968" "22,8882,1024,8561,6289,33023,8128" "23,8882,1031,6058,8561,33023,8128")
    string(FIND "${trace_text}" "${row}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "SPU trace row missing ${row}: ${root}")
    endif()
endforeach()
file(READ "${spu}" press_start HEX OFFSET 32336 LIMIT 32)
string(TOLOWER "${press_start}" press_start)
if(NOT press_start STREQUAL "0000000000000000000000000000000031043fdd360c01c140fff1fe11d15ec2")
    message(FATAL_ERROR "Primary VAB body is not at retail SPU address: ${root}")
endif()
