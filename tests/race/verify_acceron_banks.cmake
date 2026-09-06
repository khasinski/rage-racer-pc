file(MAKE_DIRECTORY "${EVIDENCE}")
if(NOT DEFINED ASSET_SOURCE)
    set(ASSET_SOURCE disc)
endif()
if(ASSET_SOURCE STREQUAL "disc")
    set(mode disc)
else()
    set(mode cache)
endif()
set(models 1 0 2)
foreach(asset RANGE 96 100 2)
    math(EXPR course "(${asset}-96)/2")
    list(GET models ${course} model)
    string(REPEAT "${model}," 10 grid)
    string(APPEND grid "${model}")
    set(log "${EVIDENCE}/acceron-${mode}-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_acceron.ini"
        --set stop.timer=120 --set "race.course=${course}" --set "race.grid=${grid}"
        --set "diagnostics.log=${log}" --set "modern.assets=${ASSET_SOURCE}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Acceron bank ${asset} race failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    foreach(required "scenario-start rival=0 [^\n]*model=${model}"
            "authored Acceron player body installed asset=38"
            "authored Acceron rival body installed asset=${asset}"
            "authored Erriso rival body installed asset=${asset}"
            "authored Esperanza rival body installed asset=${asset}")
        if(NOT trace MATCHES "${required}")
            message(FATAL_ERROR "Acceron bank ${asset} missed ${required}")
        endif()
    endforeach()
endforeach()
