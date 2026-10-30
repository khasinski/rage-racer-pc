cmake_minimum_required(VERSION 3.20)
if(NOT EXISTS "${DISC}")
    message("SKIP: provide RAGE_RUNTIME_DISC")
    return()
endif()
file(MAKE_DIRECTORY "${OUTPUT}")
foreach(aspect 21:9 32:9)
    string(REPLACE ":" "-" name "${aspect}")
    if(aspect STREQUAL "21:9")
        set(width 1680)
    else()
        set(width 2560)
    endif()
    set(image "${OUTPUT}/${name}.ppm")
    set(log "${OUTPUT}/${name}.log")
    file(REMOVE "${image}" "${log}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1
        "${GAME}" --config "${SOURCE}/rage-port.ini"
        --scenario "${SOURCE}/race-scenario.ini"
        --set "disc.image=${DISC}" --set modern.assets=disc
        --set video.renderer=modern --set "video.aspect=${aspect}"
        --set video.internal_scale=3 --set video.fps=logic
        --set video.post=none --set video.grading=off
        --set stop.scene=12 --set stop.timer=220 --set run.frames=1400
        --set "diagnostics.modern_dump=${image}"
        --set diagnostics.modern_dump_scene_id=12
        --set diagnostics.modern_dump_timer=200
        --set diagnostics.modern_dump_scene=true
        --set diagnostics.test_log=true --set "diagnostics.log=${log}"
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 120 RESULT_VARIABLE result
        OUTPUT_FILE "${OUTPUT}/${name}.stdout" ERROR_FILE "${OUTPUT}/${name}.stderr")
    if(NOT result EQUAL 0 OR NOT EXISTS "${image}")
        message(FATAL_ERROR "Ultrawide ${aspect} failed (${result}); see ${OUTPUT}")
    endif()
    file(READ "${image}" header LIMIT 40)
    if(NOT header MATCHES "^P6\n${width} 720\n255\n")
        message(FATAL_ERROR "Ultrawide ${aspect}: wrong target size: ${header}")
    endif()
    file(READ "${log}" trace)
    if(NOT trace MATCHES "native GPU pipeline ready" OR
       NOT trace MATCHES "smoke synchronized stop frame=[0-9]+ scene=12 timer=220" OR
       trace MATCHES "resource setup failed|AddressSanitizer|runtime error:")
        message(FATAL_ERROR "Ultrawide ${aspect} did not render and stop cleanly")
    endif()
    message(STATUS "Ultrawide ${aspect}: native ${width}x720 capture passed")
endforeach()
