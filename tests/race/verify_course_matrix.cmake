if(NOT DEFINED GAME OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT_ROOT)
    message(FATAL_ERROR "GAME, SOURCE and OUTPUT_ROOT are required")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/course-matrix-${id}")
file(MAKE_DIRECTORY "${root}")

# Exact classic captures prove both that the frame is populated and that the
# selected Grand Prix/Extra Grand Prix course reached its deterministic state.
set(cases
    "gp|0|5f6dc846ae40c6c4f26341d5df0578c16b2e8b1b4f2ff4d479d9de2021e54a5d"
    "gp|1|d0d7f0d9d407ae66e8c41ad4dbf0b4c555201d09ede3f49ccea6d0c929198bb5"
    "gp|2|d1f2c4e70e657c77a6fd112e42515d6d5fecb2958a0ed4a942f37c35c892897e"
    "gp|3|050f9d3c6f4e4832e1b401c430b939ea4a76a461e8f1749ab63a6025fc9a80ec"
    "extra-gp|0|c7e6a854abf897a4099f1b1c48cad1d62487e7409d226eece32b45048a3e042d"
    "extra-gp|1|599b5fa9c178fb042d9357840b269e922446efbb4291899c185dc9a311467db8"
    "extra-gp|2|95198877e34f6cd5e75a7e58a99c64bfff3d7edf4fde645b446b43cc1be13b65"
    "extra-gp|3|d661cd8e19a25c399dd657301fb5446d955b3cddcf123b2005ceac8290b27023")
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
