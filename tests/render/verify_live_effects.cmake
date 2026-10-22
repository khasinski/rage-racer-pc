# Switching the screen effects (FXAA, ambient occlusion, grading) from the
# settings panel must not rebuild the device: the resources of an effect come
# and go while the one resource generation stays.
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/live-effects-${id}")
set(assets "${root}/native assets")
file(MAKE_DIRECTORY "${assets}")
execute_process(COMMAND "${FIXTURE}" "${assets}" RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Cannot generate native assets: ${root}")
endif()
set(scenario "${root}/scenario.ini")
file(WRITE "${scenario}" "[race]\nenabled=true\nmode=grand-prix\nseries=gp\nclass=0\ncourse=0\ncar=3\n[video]\nrenderer=modern\n[modern]\nassets=${assets}\n[run]\nframes=1500\n[stop]\nscene=12\ntimer=250\n[hooks]\ntoggle_effects_frames=1000,1060,1120,1180\n[diagnostics]\nrenderer_lifecycle=true\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    "${GAME}" --scenario "${scenario}" WORKING_DIRECTORY "${SOURCE}"
    RESULT_VARIABLE result TIMEOUT 165 OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
file(WRITE "${root}/game.log" "${log}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Effect switching failed (${result}): ${root}")
endif()
string(REGEX MATCHALL "smoke effects requested=[01] post=[01] ssao=[01] grading=[01]" steps "${log}")
set(expected
    "smoke effects requested=0 post=0 ssao=0 grading=0"
    "smoke effects requested=1 post=1 ssao=1 grading=1"
    "smoke effects requested=0 post=0 ssao=0 grading=0"
    "smoke effects requested=1 post=1 ssao=1 grading=1")
if(NOT steps STREQUAL expected)
    message(FATAL_ERROR "Effects did not follow the requests: ${steps}: ${root}")
endif()
foreach(event created destroyed)
    string(REGEX MATCHALL "modern resources ${event} generation=" events "${log}")
    list(LENGTH events count)
    if(NOT count EQUAL 1)
        message(FATAL_ERROR "Expected one ${event} resource generation, got ${count}: ${root}")
    endif()
endforeach()
if(log MATCHES "disabling")
    message(FATAL_ERROR "An effect failed to come back: ${root}")
endif()
message(STATUS "Live effect switching passed: ${root}")
