# Bounded production-executable stability probe. This verifies route completion
# and resource teardown, not physical driving, image quality or monitor pacing.
cmake_minimum_required(VERSION 3.20)
foreach(argument GAME SOURCE OUTPUT DISC)
    if(NOT DEFINED ${argument})
        message(FATAL_ERROR "${argument} is required")
    endif()
endforeach()
if(NOT DEFINED COURSE)
    set(COURSE 1)
endif()
if(NOT DEFINED SERIES)
    set(SERIES extra-gp)
endif()
if(NOT DEFINED LAPS)
    set(LAPS 3)
endif()
if(NOT DEFINED SCALE)
    set(SCALE 3)
endif()
if(NOT DEFINED RENDERER)
    set(RENDERER modern)
endif()
if(NOT DEFINED STANDARD)
    set(STANDARD ntsc)
endif()
if(NOT DEFINED FPS)
    set(FPS logic)
endif()
file(MAKE_DIRECTORY "${OUTPUT}")
file(REMOVE "${OUTPUT}/runtime.log" "${OUTPUT}/frame.ppm")
file(SHA256 "${GAME}" binary_hash)
file(WRITE "${OUTPUT}/binary-sha256.txt" "${binary_hash}\n")
execute_process(COMMAND "${GAME}" --config "${SOURCE}/rage-port.ini"
    --scenario "${SOURCE}/race-scenario.ini" --set "disc.image=${DISC}"
    --set "video.renderer=${RENDERER}" --set "video.fps=${FPS}" --set "video.internal_scale=${SCALE}"
    --set video.classic_enhancements=true
    --set video.aspect=16:9 --set "timing.standard=${STANDARD}"
    --set race.class=4 --set race.car=9 --set "race.course=${COURSE}"
    --set "race.series=${SERIES}" --set autopilot.enabled=true
    --set "autopilot.laps=${LAPS}" --set autopilot.speed=4000
    --set autopilot.max_frames=20000 --set diagnostics.modern_dump_offscreen=true
    --set "diagnostics.modern_dump=${OUTPUT}/frame.ppm"
    --set diagnostics.modern_dump_frame=620 --set diagnostics.renderer_lifecycle=true
    --set diagnostics.marker_capture=false --set diagnostics.marker_history=false
    --set "diagnostics.log=${OUTPUT}/runtime.log"
    WORKING_DIRECTORY "${OUTPUT}" TIMEOUT 900 RESULT_VARIABLE result
    OUTPUT_FILE "${OUTPUT}/stdout.log" ERROR_FILE "${OUTPUT}/stderr.log")
file(WRITE "${OUTPUT}/process-result.txt" "${result}\n")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Route failed (${result}): ${OUTPUT}")
endif()
file(READ "${OUTPUT}/runtime.log" log)
set(renderer_ready "native GPU pipeline ready")
if(RENDERER STREQUAL "classic")
    set(renderer_ready "classic renderer target")
endif()
foreach(required "${renderer_ready}" "autopilot result=complete laps=${LAPS}"
        "assets_ready=0 meshes=0")
    if(NOT log MATCHES "${required}")
        message(FATAL_ERROR "Missing ${required}: ${OUTPUT}")
    endif()
endforeach()
if(log MATCHES "AddressSanitizer|runtime error:|out of memory|Out of memory|capture overflow")
    message(FATAL_ERROR "Runtime error in route: ${OUTPUT}")
endif()
if(NOT EXISTS "${OUTPUT}/frame.ppm")
    message(FATAL_ERROR "No rendered frame from route: ${OUTPUT}")
endif()
message(STATUS "${RENDERER} course ${COURSE}/${SERIES}, ${LAPS} laps and teardown verified: ${OUTPUT}")
