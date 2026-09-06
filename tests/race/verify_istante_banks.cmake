file(MAKE_DIRECTORY "${EVIDENCE}")
if(NOT DEFINED ASSET_SOURCE)
    set(ASSET_SOURCE disc)
endif()
if(ASSET_SOURCE STREQUAL "disc")
    set(mode disc)
else()
    set(mode cache)
endif()
set(models 3 3 3 0)
foreach(asset RANGE 112 118 2)
    set(class 3)
    math(EXPR course "(${asset}-112)/2")
    list(GET models ${course} model)
    string(REPEAT "${model}," 10 grid)
    string(APPEND grid "${model}")
    set(log "${EVIDENCE}/istante-${mode}-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_istante.ini"
        --set stop.timer=120 --set "race.class=${class}" --set "race.course=${course}"
        --set "race.grid=${grid}" --set "diagnostics.log=${log}" --set "modern.assets=${ASSET_SOURCE}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Istante bank ${asset} race failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    foreach(required "scenario-start rival=0 [^\n]*model=${model}"
            "authored Istante player body installed asset=62"
            "authored Istante rival body installed asset=${asset}"
            "authored Hijack rival body installed asset=${asset}"
            "authored Pegase rival body installed asset=${asset}"
            "authored Esperanza rival body installed asset=${asset}")
        if(NOT trace MATCHES "${required}")
            message(FATAL_ERROR "Istante bank ${asset} missed ${required}")
        endif()
    endforeach()
endforeach()
