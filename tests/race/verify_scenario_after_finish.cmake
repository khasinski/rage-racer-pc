# Production race transition with a synthetic finish, retaining menu recovery
# and speech ordering coverage from the previous script.
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef suffix)
set(directory "${OUTPUT_ROOT}/after-finish-${suffix}")
file(MAKE_DIRECTORY "${directory}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_SMOKE_FRAMES=3500 RAGE_PORT_SMOKE_FINISH_FRAME=2000
    RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME=3000 RAGE_PORT_SOUND_CUE_TRACE=1
    "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
    --set boot.direct=false --set race.after_finish=menu
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 195
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
file(WRITE "${directory}/game.log" "${log}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Finish scenario failed (${result}): ${directory}")
endif()
foreach(required "scenario mode=grand-prix"
        "finish follow-up queued cue=0x2b"
        "finish follow-up released cue=0x2b"
        "sound cue=0x2b" "scenario race finished after_finish=menu"
        "scenario automation stopped after finish" "scene=17 frontend=3")
    string(FIND "${log}" "${required}" position)
    if(position LESS 0)
        message(FATAL_ERROR "Missing ${required}: ${directory}")
    endif()
endforeach()
string(FIND "${log}" "finish follow-up queued cue=0x2b" queued)
string(FIND "${log}" "finish follow-up released cue=0x2b" released)
string(FIND "${log}" "sound cue=0x2b" spoken)
if(NOT queued LESS released OR NOT released LESS spoken)
    message(FATAL_ERROR "Finish speech escaped queue ordering: ${directory}")
endif()
string(SUBSTRING "${log}" ${queued} -1 after_finish)
if(after_finish MATCHES "sound cue=0x2a")
    message(FATAL_ERROR "Final-stretch encouragement repeated after finish: ${directory}")
endif()
string(FIND "${log}" "scenario automation stopped after finish" stopped)
string(SUBSTRING "${log}" ${stopped} -1 after_stop)
if(after_stop MATCHES "scenario confirm")
    message(FATAL_ERROR "Scenario continued automatic menu input: ${directory}")
endif()
message(STATUS "Finish speech and menu recovery verified: ${directory}")
