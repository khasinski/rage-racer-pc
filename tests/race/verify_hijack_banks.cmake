file(MAKE_DIRECTORY "${EVIDENCE}")
if(NOT DEFINED ASSET_SOURCE)
    set(ASSET_SOURCE disc)
endif()
if(ASSET_SOURCE STREQUAL "disc")
    set(mode disc)
else()
    set(mode cache)
endif()
if(NOT DEFINED ASSETS)
    set(ASSETS 94 112 114 116 118 120 122 124 126)
endif()
set(models 1 0 2 2)
foreach(asset IN LISTS ASSETS)
    math(EXPR class "(${asset}-88)/8")
    math(EXPR course "((${asset}-88)%8)/2")
    list(GET models ${course} model)
    string(REPEAT "${model}," 10 grid)
    string(APPEND grid "${model}")
    set(log "${EVIDENCE}/hijack-${mode}-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_hijack.ini"
        --set stop.timer=120 --set "race.class=${class}" --set "race.course=${course}"
        --set "race.grid=${grid}" --set "diagnostics.log=${log}" --set "modern.assets=${ASSET_SOURCE}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Hijack bank ${asset} race failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    foreach(required "scenario-start rival=0 [^\n]*model=${model}"
            "authored Hijack player body installed asset=52"
            "authored Hijack rival body installed asset=${asset}"
            "authored Pegase rival body installed asset=${asset}"
            "authored Esperanza rival body installed asset=${asset}")
        if(NOT trace MATCHES "${required}")
            message(FATAL_ERROR "Hijack bank ${asset} missed ${required}")
        endif()
    endforeach()
endforeach()
