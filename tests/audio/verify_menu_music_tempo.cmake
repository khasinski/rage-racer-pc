if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

function(run_menu_sequence standard notes_out)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env SDL_AUDIODRIVER=dummy
            RAGE_PORT_SMOKE_FRAMES=1400 RAGE_PORT_SMOKE_AUDIO_METRICS=1
            RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,950:CROSS,1100:CROSS,1200:CROSS
            RAGE_PORT_SMOKE_STOP_SCENE=10 RAGE_PORT_SMOKE_STOP_SCENE_TIMER=40
            "${GAME}" --set video.renderer=classic --set timing.standard=${standard}
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 120
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${standard} menu-tempo run failed (${result}):\n${log}")
    endif()
    string(REGEX MATCH "seq_notes=([0-9]+) " notes_match "${log}")
    if(NOT notes_match)
        message(FATAL_ERROR "${standard} menu-tempo run reported no audio metrics:\n${log}")
    endif()
    set(${notes_out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

# Both frame-scripted runs cover the same game frames. The menu SEQ must follow
# those frames on PAL too: a synthetic 60 Hz clock makes its notes 20% fast.
run_menu_sequence(pal pal_notes)
run_menu_sequence(ntsc ntsc_notes)
if(pal_notes EQUAL 0 OR ntsc_notes EQUAL 0)
    message(FATAL_ERROR "Menu sequence never played: pal=${pal_notes}, ntsc=${ntsc_notes}")
endif()

# The established PAL baseline is 124 notes for this route. Allow the previous
# timing slack while requiring PAL and NTSC to remain frame-for-frame aligned.
math(EXPR note_difference "${pal_notes} - ${ntsc_notes}")
if(note_difference LESS -12 OR note_difference GREATER 12)
    message(FATAL_ERROR "Menu sequence rate differs by standard: pal=${pal_notes}, ntsc=${ntsc_notes}")
endif()
if(pal_notes LESS 112 OR pal_notes GREATER 136)
    message(FATAL_ERROR "Menu sequence missed the PAL frame-clock baseline: pal=${pal_notes}")
endif()
message(STATUS "Menu sequence kept its tempo: pal=${pal_notes}, ntsc=${ntsc_notes}")
