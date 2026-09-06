# Compiled smoke + PCM verifier; no Python dependency. This checks direct FMV
# playback and sector-derived pacing, not the gameplay path that awards a class.
if(NOT DEFINED STREAM)
    set(STREAM 5)
endif()
if(NOT STREAM MATCHES "^([0-9]|10)$")
    message(FATAL_ERROR "STREAM must be a retail FMV index, 0 through 10")
endif()
if(STREAM EQUAL 0)
    set(smoke_frames 9540) # 2160 * 4 + 900, covers both retail intro lengths
elseif(STREAM EQUAL 10)
    set(smoke_frames 6900) # 1500 * 4 + 900
else()
    set(smoke_frames 2100) # 300 * 4 + 900
endif()
if(NOT DEFINED DISC OR DISC STREQUAL "")
    if(NOT "$ENV{RAGE_PORT_DISC_IMAGE}" STREQUAL "")
        set(DISC "$ENV{RAGE_PORT_DISC_IMAGE}")
    elseif(NOT "$ENV{RAGE_PORT_DISC_CUE}" STREQUAL "")
        set(DISC "$ENV{RAGE_PORT_DISC_CUE}")
    else()
        set(DISC "${SOURCE}/disc/PAL/Rage Racer (Europe)/Rage Racer (Europe).cue")
    endif()
endif()
if(NOT EXISTS "${DISC}")
    message("SKIP: no disc image for modern FMV audio")
    return()
endif()
get_filename_component(DISC "${DISC}" ABSOLUTE BASE_DIR "${SOURCE}")
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef run_id)
set(output "${OUTPUT_ROOT}/modern-fmv-${run_id}")
file(MAKE_DIRECTORY "${output}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    "SDL_AUDIODRIVER=dummy"
    "PSYZ_AUDIO_PCM_DUMP=${output}/audio.s16le"
    "RAGE_PORT_SMOKE_FRAMES=${smoke_frames}"
    "RAGE_PORT_FMV_TRACE=1"
    "RAGE_PORT_SMOKE_AUDIO_METRICS=1"
    "${GAME}" --set "disc.image=${DISC}"
    --set "diagnostics.fmv_stream=${STREAM}" --set video.renderer=modern
    --set diagnostics.marker_capture=false --set diagnostics.marker_history=false
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 540
    OUTPUT_FILE "${output}/game.log" ERROR_FILE "${output}/game.log"
    RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Modern FMV failed (${result}); ${output}/game.log")
endif()
file(READ "${output}/game.log" log)
foreach(required "active=modern" "fmv xa start:" "fmv xa end")
    if(NOT log MATCHES "${required}")
        message(FATAL_ERROR "Missing ${required}; ${output}/game.log")
    endif()
endforeach()
string(REGEX MATCHALL "fmv frame=[0-9]+ " trace "${log}")
# The title screen may replay the intro during the remaining smoke ticks.
# Require the complete first run, never add a partial replay to its frame count.
set(frames "")
foreach(entry IN LISTS trace)
    if(entry STREQUAL "fmv frame=0 " AND NOT "${frames}" STREQUAL "")
        break()
    endif()
    list(APPEND frames "${entry}")
endforeach()
list(LENGTH frames count)
# Retail promotion encodes 150 frames in PAL, 300 in both NTSC editions;
# the opening differs too, while the ending has 1500 frames in all editions.
# Do not derive the expectation from the game's reported stream frame count.
if(log MATCHES "disc SCES_006.50 region=PAL")
    set(expected_frames 150)
    if(STREAM EQUAL 0)
        set(expected_frames 1800)
    endif()
elseif(log MATCHES "disc (SLUS_004.03 region=NTSC-U|SLPS_006.00 region=NTSC-J)")
    set(expected_frames 300)
    if(STREAM EQUAL 0)
        set(expected_frames 2160)
    endif()
else()
    message(FATAL_ERROR "Unrecognized retail identity; ${output}")
endif()
if(STREAM EQUAL 10)
    set(expected_frames 1500)
endif()
if(NOT count EQUAL expected_frames)
    message(FATAL_ERROR "Stream ${STREAM}: expected ${expected_frames} frames, got ${count}; ${output}")
endif()
math(EXPR final_frame "${expected_frames} - 1")
foreach(frame RANGE 0 ${final_frame})
    list(GET frames ${frame} actual)
    if(NOT actual STREQUAL "fmv frame=${frame} ")
        message(FATAL_ERROR "Stream ${STREAM}: out-of-order frame ${frame}; ${output}")
    endif()
endforeach()
if(NOT log MATCHES "audio metrics: frames=([0-9]+) energy=([0-9]+)")
    message(FATAL_ERROR "Missing mixer metrics; ${output}")
endif()
execute_process(COMMAND "${PCM_CHECK}" "${output}/audio.s16le"
    "${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}"
    RESULT_VARIABLE result OUTPUT_VARIABLE metrics ERROR_VARIABLE error)
file(WRITE "${output}/pcm-check.log" "${metrics}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "PCM verification failed: ${metrics}${error}; ${output}")
endif()
if(DEFINED PACING_CHECK)
    execute_process(COMMAND "${PACING_CHECK}" "${DISC}" "${STREAM}" "${output}/game.log"
        RESULT_VARIABLE result OUTPUT_VARIABLE pacing ERROR_VARIABLE error)
    file(WRITE "${output}/pacing-check.log" "${pacing}${error}")
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "Sector/XA pacing failed: ${pacing}${error}; ${output}")
    endif()
    # Negative oracle check: preserve every frame/sector and all sound metrics,
    # but make the last picture arrive much later. Count/PCM-only checks pass
    # this log; the independent disc timing oracle must reject it.
    string(REGEX MATCH "fmv frame=${final_frame} vblank=([0-9]+)" last_trace "${log}")
    math(EXPR delayed_tick "${CMAKE_MATCH_1} * 2")
    string(REPLACE "${last_trace}" "fmv frame=${final_frame} vblank=${delayed_tick}"
        delayed "${log}")
    file(WRITE "${output}/delayed-picture.log" "${delayed}")
    execute_process(COMMAND "${PACING_CHECK}" "${DISC}" "${STREAM}" "${output}/delayed-picture.log"
        RESULT_VARIABLE rejected OUTPUT_VARIABLE negative ERROR_VARIABLE error)
    file(WRITE "${output}/negative-pacing-check.log" "${negative}${error}")
    if(NOT rejected STREQUAL "1")
        message(FATAL_ERROR "Pacing oracle did not reject delayed picture (${rejected}); ${output}")
    endif()
endif()
message(STATUS "Modern FMV ${STREAM}: ${expected_frames} frames, XA finished, ${metrics}Evidence: ${output}")
