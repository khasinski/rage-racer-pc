if(NOT DEFINED GAME OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT_ROOT)
    message(FATAL_ERROR "GAME, SOURCE and OUTPUT_ROOT are required")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/course-matrix-${id}")
file(MAKE_DIRECTORY "${root}")

# References reviewed against pre-rebase captures: 0–14 pixels changed per frame
# (maximum channel delta 9/255); see docs/release-validation-2026-11-03.md.
# Exact classic captures prove both that the frame is populated and that the
# selected Grand Prix/Extra Grand Prix course reached its deterministic state.
set(cases
    "gp|0|b7e298085ac69ead3a6c7c5fbfee04acc609495a9cac60ff17b6f26f73795746"
    "gp|1|c1a36f3d44d67abc93febf4a4313f0030edbf64a5fa50ba8b2a54a0158808849"
    "gp|2|3883827a43282f4a28cbcc2e5d0fc44b7407c4ea0a45188edfcbf4fd02dfdce1"
    "gp|3|050f9d3c6f4e4832e1b401c430b939ea4a76a461e8f1749ab63a6025fc9a80ec"
    "extra-gp|0|3060a3f20ea0ef850885ba470fe45d2e319c65c23378a2968bdb558dbc3c0755"
    "extra-gp|1|a6fa0f1803520a19a9aecb613374571549c7814d70135e833171a758c0060fbf"
    "extra-gp|2|964335834e236a156a9285670b80898d0a4f9d72c57c2c89554979e80f2b7ea9"
    "extra-gp|3|e2cde7e7a708482f287d7e19f982abeebf882c305dd9e65fb8960453d5746cde")
foreach(case IN LISTS cases)
    string(REPLACE "|" ";" fields "${case}")
    list(GET fields 0 series)
    list(GET fields 1 course)
    list(GET fields 2 expected_hash)
    set(capture "${root}/${series}-${course}.ppm")
    set(scenario "${root}/${series}-${course}.ini")
    file(WRITE "${scenario}" "[video]\nrenderer = classic\n\n[race]\nenabled = true\nmode = grand-prix\nseries = ${series}\nclass = 0\ncourse = ${course}\ncar = 3\n\n[run]\nframes = 2500\n\n[stop]\nscene = 12\ntimer = 56\n\n[capture]\npath = ${capture}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
        "${GAME}" --scenario "${scenario}" WORKING_DIRECTORY "${SOURCE}" TIMEOUT 135
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    set(reported_series "${series}")
    if(series STREQUAL "gp")
        set(reported_series "grand-prix")
    endif()
    if(NOT result EQUAL 0 OR NOT EXISTS "${capture}" OR
       NOT log MATCHES "series=${reported_series} class=0 course=${course} car=3" OR
       NOT log MATCHES "scene 12")
        file(WRITE "${root}/${series}-${course}.log" "${log}")
        message(FATAL_ERROR "Course matrix failed for ${series}/${course}: ${root}")
    endif()
    file(SHA256 "${capture}" actual_hash)
    if(NOT actual_hash STREQUAL expected_hash)
        message(FATAL_ERROR "Classic pixels changed for ${series}/${course}: ${actual_hash}; expected ${expected_hash}; ${root}")
    endif()
endforeach()
message(STATUS "All Grand Prix and Extra Grand Prix course captures matched")
