execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=2200 RAGE_PORT_SMOKE_AUDIO_METRICS=1
    RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,1015:CROSS,1165:CROSS,1265:CROSS,1533-10000:CROSS
    "${GAME}" WORKING_DIRECTORY "${SOURCE}" TIMEOUT 180 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Engine audio run failed (${result})\n${log}")
endif()
string(REGEX MATCH "pitch_updates=([0-9]+)" pitch_match "${log}")
if(NOT pitch_match OR CMAKE_MATCH_1 LESS 500)
    message(FATAL_ERROR "Pitched effect voices stalled: ${log}")
endif()
string(REGEX MATCH "reverb_in=([0-9]+) reverb_out=([0-9]+) reverb_tail=([0-9]+)" reverb_match "${log}")
if(NOT reverb_match OR CMAKE_MATCH_1 EQUAL 0 OR CMAKE_MATCH_2 EQUAL 0 OR CMAKE_MATCH_3 EQUAL 0)
    message(FATAL_ERROR "Race reverb did not produce a wet tail: ${log}")
endif()
