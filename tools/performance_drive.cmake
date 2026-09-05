# Repeatable Linux modern-renderer profiling with real local disc data.
cmake_minimum_required(VERSION 3.20)
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This profiling harness currently supports Linux")
endif()
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
foreach(required GAME DISC CONFIG)
    if(NOT DEFINED ${required} OR NOT EXISTS "${${required}}")
        message(FATAL_ERROR "Pass -D${required}=an-existing-path")
    endif()
    get_filename_component(${required} "${${required}}" ABSOLUTE)
endforeach()
foreach(pair LAPS:2 RACES:0 CLASS:0 COURSE:0 TIMEOUT:600)
    string(REPLACE ":" ";" fields "${pair}")
    list(GET fields 0 key)
    list(GET fields 1 default)
    if(NOT DEFINED ${key})
        set(${key} "${default}")
    endif()
    if(NOT "${${key}}" MATCHES "^[0-9]+$")
        message(FATAL_ERROR "${key} must be an unsigned integer")
    endif()
endforeach()
if(LAPS LESS 1 OR LAPS GREATER 100 OR RACES GREATER 100 OR CLASS GREATER 5 OR
   COURSE GREATER 3 OR TIMEOUT LESS 1 OR TIMEOUT GREATER 3600)
    message(FATAL_ERROR "Profiling limits out of range")
endif()
foreach(option TRACE PREWARM VERIFY_VRAM)
    if(NOT DEFINED ${option})
        set(${option} OFF)
        if(option STREQUAL "PREWARM")
            set(${option} ON)
        endif()
    endif()
endforeach()
set(trace false)
set(prewarm false)
if(TRACE)
    set(trace true)
endif()
if(PREWARM)
    set(prewarm true)
endif()
if(NOT DEFINED OUTPUT)
    set(OUTPUT "${ROOT}/build/performance-drive")
endif()
string(TIMESTAMP stamp "%Y%m%d-%H%M%S")
string(RANDOM LENGTH 6 ALPHABET 0123456789abcdef suffix)
get_filename_component(session "${OUTPUT}/${stamp}-${suffix}" ABSOLUTE)
file(MAKE_DIRECTORY "${session}/state")
file(SHA256 "${GAME}" game_hash)
file(SHA256 "${CONFIG}" config_hash)
set(oracleEnv)
if(VERIFY_VRAM)
    list(APPEND oracleEnv PSYZ_VERIFY_VRAM_READ_CACHE=1)
endif()
message(STATUS "Profiling session: ${session}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    --unset=RAGE_TEST_SCENARIO --unset=PSYZ_VERIFY_VRAM_READ_CACHE
    "XDG_STATE_HOME=${session}/state" ${oracleEnv}
    /usr/bin/time -v -o "${session}/time.txt"
    "${GAME}" --config "${CONFIG}" --scenario "${ROOT}/race-scenario.ini"
    --set "disc.image=${DISC}" --set video.renderer=modern
    --set "race.class=${CLASS}" --set "race.course=${COURSE}"
    --set start.freeze=false --set race.after_finish=repeat
    --set autopilot.enabled=true --set autopilot.speed=6000
    --set "autopilot.laps=${LAPS}" --set "autopilot.races=${RACES}"
    --set diagnostics.performance=true --set "diagnostics.performance_trace=${trace}"
    --set "diagnostics.texture_prewarm=${prewarm}"
    --set diagnostics.marker_capture=false --set diagnostics.marker_history=false
    --set diagnostics.renderdoc=false --set "diagnostics.log=${session}/game.log"
    WORKING_DIRECTORY "${ROOT}" TIMEOUT ${TIMEOUT} RESULT_VARIABLE result
    OUTPUT_FILE "${session}/launcher.log" ERROR_FILE "${session}/launcher-errors.log")
if(NOT EXISTS "${session}/game.log")
    message(FATAL_ERROR "No game log; process result: ${result}")
endif()
file(READ "${session}/game.log" log)
set(completion "laps=${LAPS}")
if(RACES GREATER 0)
    set(completion "races=${RACES}")
endif()
if(NOT result EQUAL 0 OR NOT log MATCHES "autopilot result=complete ${completion}")
    message(FATAL_ERROR "Incomplete route: ${result}; see ${session}")
endif()
if(NOT log MATCHES "modern renderer target" OR log MATCHES "vram-read-cache verify=MISMATCH")
    message(FATAL_ERROR "Renderer/cache verification failed; see ${session}")
endif()
if(VERIFY_VRAM AND NOT log MATCHES "vram-read-cache verify=match")
    message(FATAL_ERROR "No cache-oracle comparisons; see ${session}")
endif()
file(WRITE "${session}/result.txt"
    "completed=${completion}\ngame_sha256=${game_hash}\nconfig_sha256=${config_hash}\ntrace=${trace}\nprewarm=${prewarm}\nvram_oracle=${VERIFY_VRAM}\nvisual_correctness=not_automatically_asserted\n")
message(STATUS "Complete: ${session}")
