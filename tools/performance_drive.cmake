# Repeatable Linux modern-renderer profiling with real local disc data.
cmake_minimum_required(VERSION 3.20)
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This profiling harness currently supports Linux")
endif()
# A locked Wayland desktop can throttle swapchain acquisition even when the
# display reports 120 Hz. Do not record those runs as foreground performance.
if(NOT DEFINED ENV{SDL_VIDEODRIVER} OR "$ENV{SDL_VIDEODRIVER}" MATCHES "^(|wayland|x11)$")
    find_program(RAGE_PROFILE_QDBUS NAMES qdbus6 qdbus)
    if(RAGE_PROFILE_QDBUS)
        execute_process(COMMAND "${RAGE_PROFILE_QDBUS}"
            org.freedesktop.ScreenSaver /ScreenSaver
            org.freedesktop.ScreenSaver.GetActive
            RESULT_VARIABLE lock_query_result OUTPUT_VARIABLE screen_locked
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET TIMEOUT 3)
        if(lock_query_result EQUAL 0 AND screen_locked STREQUAL "true")
            message(FATAL_ERROR
                "Unlock the desktop before profiling: the screen saver is active and may throttle presentation")
        endif()
    endif()
endif()
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
foreach(required GAME DISC CONFIG)
    if(NOT DEFINED ${required} OR NOT EXISTS "${${required}}")
        message(FATAL_ERROR "Pass -D${required}=an-existing-path")
    endif()
    get_filename_component(${required} "${${required}}" ABSOLUTE)
endforeach()
foreach(pair LAPS:2 RACES:0 CLASS:0 COURSE:0 TIMEOUT:600 RESTARTS:0)
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
   COURSE GREATER 3 OR TIMEOUT LESS 1 OR TIMEOUT GREATER 3600 OR RESTARTS GREATER 10)
    message(FATAL_ERROR "Profiling limits out of range")
endif()
foreach(option TRACE PREWARM VERIFY_VRAM VERIFY_LIFECYCLE)
    if(NOT DEFINED ${option})
        set(${option} OFF)
        if(option STREQUAL "PREWARM")
            set(${option} ON)
        endif()
    endif()
endforeach()
if(DEFINED EXPECT_REGION AND NOT EXPECT_REGION MATCHES "^(PAL|NTSC-U|NTSC-J)$")
    message(FATAL_ERROR "EXPECT_REGION must be PAL, NTSC-U or NTSC-J")
endif()
set(trace false)
set(prewarm false)
set(lifecycle false)
if(VERIFY_LIFECYCLE OR RESTARTS GREATER 0)
    set(lifecycle true)
endif()
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
    --set "diagnostics.renderer_lifecycle=${lifecycle}"
    --set "diagnostics.renderer_restart_count=${RESTARTS}"
    --set "diagnostics.texture_prewarm=${prewarm}"
    --set diagnostics.marker_capture=false --set diagnostics.marker_history=false
    --set "diagnostics.log=${session}/game.log"
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
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Incomplete route: ${result}; see ${session}")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/verify_route_completion.cmake")
rage_verify_route_completion("${log}" "${completion}" "${RACES}")
if(NOT log MATCHES "modern renderer target" OR log MATCHES "vram-read-cache verify=MISMATCH")
    message(FATAL_ERROR "Renderer/cache verification failed; see ${session}")
endif()
if(VERIFY_VRAM AND NOT log MATCHES "vram-read-cache verify=match")
    message(FATAL_ERROR "No cache-oracle comparisons; see ${session}")
endif()
if(DEFINED EXPECT_REGION)
    if(EXPECT_REGION STREQUAL "PAL")
        set(expected_timing "pal base_hz=50")
    else()
        set(expected_timing "ntsc base_hz=60")
    endif()
    if(NOT log MATCHES "disc [^\r\n]+ region=${EXPECT_REGION}[\r\n]" OR
       NOT log MATCHES "timing standard selected from disc region=${EXPECT_REGION}[\r\n]" OR
       NOT log MATCHES "timing=${expected_timing}[\r\n]")
        message(FATAL_ERROR "Disc region/automatic timing mismatch: expected ${EXPECT_REGION}; see ${session}")
    endif()
endif()
if(VERIFY_LIFECYCLE)
    string(REGEX MATCHALL "modern session shutdown assets_ready=0 meshes=0" shutdowns "${log}")
    list(LENGTH shutdowns shutdown_count)
    if(NOT shutdown_count EQUAL 1 OR NOT log MATCHES "modern resources destroyed generation=")
        message(FATAL_ERROR "Missing or repeated session/resource teardown; see ${session}")
    endif()
    string(FIND "${log}" "modern resources destroyed generation=" gpu_release REVERSE)
    string(FIND "${log}" "modern session shutdown assets_ready=0 meshes=0" asset_release)
    if(gpu_release GREATER_EQUAL asset_release)
        message(FATAL_ERROR "Session teardown preceded GPU-resource release; see ${session}")
    endif()
endif()
if(RESTARTS GREATER 0)
    string(REGEX MATCHALL "modern presentation restart index=[0-9]+ result=ok" restart_events "${log}")
    list(LENGTH restart_events restart_count)
    string(REGEX MATCHALL "modern resources created generation=" generations "${log}")
    list(LENGTH generations generation_count)
    if(NOT restart_count EQUAL RESTARTS OR generation_count LESS_EQUAL RESTARTS OR
       log MATCHES "modern presentation restart index=[0-9]+ result=failed")
        message(FATAL_ERROR "Presentation restart or resource recreation failed; see ${session}")
    endif()
endif()
file(WRITE "${session}/result.txt"
    "completed=${completion}\ngame_sha256=${game_hash}\nconfig_sha256=${config_hash}\ntrace=${trace}\nprewarm=${prewarm}\npresentation_restarts=${RESTARTS}\nsdl_video_driver_request=$ENV{SDL_VIDEODRIVER}\nexpected_region=${EXPECT_REGION}\nvram_oracle=${VERIFY_VRAM}\nlifecycle_oracle=${VERIFY_LIFECYCLE}\nvisual_correctness=not_automatically_asserted\n")
message(STATUS "Complete: ${session}")
