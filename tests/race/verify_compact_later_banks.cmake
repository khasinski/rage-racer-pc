file(MAKE_DIRECTORY "${EVIDENCE}")
if(NOT DEFINED ASSET_SOURCE)
    set(ASSET_SOURCE disc)
endif()
if(ASSET_SOURCE STREQUAL "disc")
    set(mode disc)
else()
    set(mode cache)
endif()
foreach(asset 94 102 104 106 108 110 112 114 116 118 120 122 124 126)
    math(EXPR class "(${asset}-88)/8")
    math(EXPR course "(${asset}-88-${class}*8)/2")
    set(log "${EVIDENCE}/compact-later-${mode}-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_compact_later.ini"
        --set stop.timer=120 --set "race.class=${class}" --set "race.course=${course}"
        --set "diagnostics.log=${log}" --set "modern.assets=${ASSET_SOURCE}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Compact bank ${asset} failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    foreach(index RANGE 0 6)
        math(EXPR model "${index}+4")
        if(NOT trace MATCHES "scenario-start rival=${index} [^\n]*model=${model}")
            message(FATAL_ERROR "Compact bank ${asset} missed palette variant ${model}")
        endif()
    endforeach()
    foreach(required "authored Esperanza player body installed asset=28"
            "authored Esperanza rival body installed asset=${asset}"
            "authored Compact A rival body installed asset=${asset}"
            "authored Compact B rival body installed asset=${asset}"
            "authored Compact C rival body installed asset=${asset}")
        if(NOT trace MATCHES "${required}")
            message(FATAL_ERROR "Compact bank ${asset} missed ${required}")
        endif()
    endforeach()
endforeach()
