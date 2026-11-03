string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/brightness-${id}")
file(MAKE_DIRECTORY "${root}/assets")
execute_process(COMMAND "${FIXTURE}" "${root}/assets" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot prepare brightness fixture")
endif()
foreach(case modern-1.0 modern-1.15 modern-1.5 classic-0.5 classic-1.5
             classic-0.5-graded classic-1.5-graded)
    string(REPLACE "-" ";" fields "${case}")
    list(GET fields 0 renderer)
    list(GET fields 1 brightness)
    set(grading off)
    if(case MATCHES "-graded$")
        set(grading vibrant)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
        "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
        --set "modern.assets=${root}/assets" --set "video.renderer=${renderer}"
        --set "video.brightness=${brightness}" --set "video.grading=${grading}"
        --set video.post=none --set video.ssao=off --set video.shadows=off
        --set video.internal_scale=1 --set video.aspect=4:3 --set video.fps=logic
        --set race.course=0 --set race.class=0 --set race.car=3
        --set start.freeze=true --set input.disable_host=true
        --set run.frames=1600 --set stop.scene=12 --set stop.timer=440
        --set "diagnostics.modern_dump=${root}/${case}.ppm"
        --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=430
        --set diagnostics.modern_dump_offscreen=true
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    file(WRITE "${root}/${case}.log" "${output}${error}")
    if(NOT result EQUAL 0 OR NOT EXISTS "${root}/${case}.ppm")
        message(FATAL_ERROR "Brightness capture failed: ${case}; ${root}")
    endif()
    file(SHA256 "${root}/${case}.ppm" hash)
    set("hash_${case}" "${hash}")
endforeach()
if(NOT hash_classic-0.5 STREQUAL hash_classic-1.5)
    message(FATAL_ERROR "Brightness changed classic pixels: ${root}")
endif()
if(NOT hash_classic-0.5-graded STREQUAL hash_classic-1.5-graded)
    message(FATAL_ERROR "Brightness changed graded classic pixels: ${root}")
endif()
if(hash_modern-1.0 STREQUAL hash_modern-1.15 OR
   hash_modern-1.15 STREQUAL hash_modern-1.5)
    message(FATAL_ERROR "Modern brightness did not change the output: ${root}")
endif()
message(STATUS "Modern brightness works independently of grading; classic is byte-identical: ${root}")
