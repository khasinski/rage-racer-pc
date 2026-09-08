cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED GAME OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "GAME, SOURCE and OUTPUT are required")
endif()
if(NOT DEFINED DISC OR NOT EXISTS "${DISC}")
    set(DISC "$ENV{RAGE_PORT_DISC_CUE}")
endif()
if(NOT EXISTS "${DISC}")
    message("SKIP: provide DISC or RAGE_PORT_DISC_CUE")
    return()
endif()
if(NOT DEFINED MODE)
    set(MODE wide)
endif()
set(fps logic)
set(scale 3)
set(aspect 16:9)
set(width 1280)
set(height 720)
if(MODE STREQUAL "high_fps")
    set(fps 120)
    set(scale 4.5)
    set(width 1920)
    set(height 1080)
elseif(MODE STREQUAL "standard")
    set(aspect 4:3)
    set(scale 1)
    set(width 320)
    set(height 240)
elseif(NOT MODE STREQUAL "wide")
    message(FATAL_ERROR "Unknown MODE ${MODE}")
endif()
file(MAKE_DIRECTORY "${OUTPUT}")
set(log "${OUTPUT}/runtime.log")
set(image "${OUTPUT}/frame.ppm")
# Runtime logs append sessions; stale failures/images must not contaminate a
# rerun or make an interrupted run appear successful.
file(REMOVE "${log}" "${image}")
execute_process(COMMAND "${GAME}" --config "${SOURCE}/rage-port.ini"
    --scenario "${SOURCE}/race-scenario.ini"
    --set "disc.image=${DISC}" --set video.renderer=classic
    --set video.classic_enhancements=true --set "video.fps=${fps}"
    --set "video.internal_scale=${scale}" --set "video.aspect=${aspect}"
    --set video.post=none --set video.grading=off --set video.texture_filter=nearest
    --set race.course=1 --set race.class=4 --set race.car=9
    --set race.series=extra-gp --set timing.standard=ntsc
    --set autopilot.enabled=true --set autopilot.speed=4000
    --set stop.scene=12 --set stop.timer=450
    --set diagnostics.classic_trace=true --set diagnostics.scene_trace=1
    --set diagnostics.classic_dump_midpoint=true
    --set "diagnostics.modern_dump=${image}" --set diagnostics.modern_dump_frame=620
    --set diagnostics.modern_dump_offscreen=true --set "diagnostics.log=${log}"
    WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE result
    OUTPUT_FILE "${OUTPUT}/stdout.log" ERROR_FILE "${OUTPUT}/stderr.log" TIMEOUT 90)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Classic ${MODE} process failed: ${result}; see ${OUTPUT}")
endif()
file(READ "${image}" header LIMIT 32)
if(NOT header MATCHES "^P6\n${width} ${height}\n255\n")
    message(FATAL_ERROR "Classic target dimensions do not match ${width}x${height}")
endif()
file(READ "${log}" trace)
if(NOT trace MATCHES "stopping at scene 12 timer 450" OR
   NOT trace MATCHES "classic renderer target ${width}x${height}")
    message(FATAL_ERROR "Classic run did not reach the expected scene and target")
endif()
string(REGEX MATCHALL "overflow=[0-9,]+" overflows "${trace}")
foreach(overflow IN LISTS overflows)
    if(NOT overflow STREQUAL "overflow=0,0,0,0,0")
        message(FATAL_ERROR "Incomplete classic stream: ${overflow}")
    endif()
endforeach()
if(trace MATCHES "resource setup failed|submit failed|runtime error:|AddressSanitizer")
    message(FATAL_ERROR "Classic capture/render failure; see ${log}")
endif()
if(MODE STREQUAL "high_fps")
    # Distinct vertex hashes within one logic frame prove actual geometric
    # interpolation, rather than merely submitting the same image repeatedly.
    string(REGEX MATCHALL "classic-frame frame=[0-9]+ t=[0-9.]+ packets=[0-9]+ matched=[0-9]+ vertices=[0-9]+ spans=[0-9]+ xy_hash=[0-9a-f]+" frames "${trace}")
    set(previousFrame "")
    set(previousHash "")
    set(changed 0)
    foreach(line IN LISTS frames)
        string(REGEX REPLACE ".*frame=([0-9]+).*" "\\1" frame "${line}")
        string(REGEX REPLACE ".*xy_hash=([0-9a-f]+).*" "\\1" hash "${line}")
        if(frame STREQUAL previousFrame AND NOT hash STREQUAL previousHash AND
           line MATCHES "matched=[1-9][0-9]*")
            math(EXPR changed "${changed}+1")
        endif()
        set(previousFrame "${frame}")
        set(previousHash "${hash}")
    endforeach()
    if(changed LESS 100)
        message(FATAL_ERROR "Too few distinct interpolated presentations: ${changed}")
    endif()
    message(STATUS "Classic: ${changed} distinct presentations between logic ticks")
endif()
message(STATUS "Classic ${MODE}: ${width}x${height}, scene reached, capture complete")
