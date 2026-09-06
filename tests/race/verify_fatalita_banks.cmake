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
foreach(asset RANGE 102 110 2)
    if(asset EQUAL 102)
        set(class 1)
        set(course 3)
    else()
        set(class 2)
        math(EXPR course "(${asset}-104)/2")
    endif()
    list(GET models ${course} model)
    string(REPEAT "${model}," 10 grid)
    string(APPEND grid "${model}")
    set(log "${EVIDENCE}/fatalita-${mode}-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_fatalita.ini"
        --set stop.timer=120 --set "race.class=${class}" --set "race.course=${course}"
        --set "race.grid=${grid}" --set "diagnostics.log=${log}" --set "modern.assets=${ASSET_SOURCE}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Fatalita bank ${asset} race failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    foreach(required "scenario-start rival=0 [^\n]*model=${model}"
            "authored Fatalita player body installed asset=56"
            "authored Fatalita rival body installed asset=${asset}"
            "authored Bayonet rival body installed asset=${asset}"
            "authored Abeille rival body installed asset=${asset}"
            "authored Esperanza rival body installed asset=${asset}")
        if(NOT trace MATCHES "${required}")
            message(FATAL_ERROR "Fatalita bank ${asset} missed ${required}")
        endif()
    endforeach()
endforeach()
