if(NOT DEFINED GAME OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT_ROOT)
    message(FATAL_ERROR "GAME, SOURCE and OUTPUT_ROOT are required")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/course-matrix-${id}")
file(MAKE_DIRECTORY "${root}")

# Exact classic captures prove both that the frame is populated and that the
# selected Grand Prix/Extra Grand Prix course reached its deterministic state.
set(cases
    "gp|0|da080b332d8d875330a8ab51c069d456271ac6f60e512dfea9180d167b02e769"
    "gp|1|899c3a8382c274c5a24a4fdd79574424a03c651a123a48751006703cdefba795"
    "gp|2|a6e27cce24ce4f7e663c1022dc0a978e6a1f1e45a6027379a933fb2cc03d7af3"
    "gp|3|2dd8e9fc72c9b2d1a5bf71bef34e382e29f9c0232d26c9747b3282e8e8758121"
    "extra-gp|0|50b97585f7aa7c9b75de5d6d8652a1bf9dc9b0476e2d2c3378935c2c18f62293"
    "extra-gp|1|c221cbe8f75dff984d978c4f0c18a94d1c8539a8a23c6de7c8995e305a3a9a24"
    "extra-gp|2|ddaf5d412cdfcfbd8ff1a3b12eb481fa96ce40daeda7b8d6027441e6d286da85"
    "extra-gp|3|a81c48c7646fb2b61ae4abd250565036bdc48ff2807dfaca75489588271f9ca0")
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
