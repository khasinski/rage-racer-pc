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
foreach(asset RANGE 128 134 2)
    math(EXPR course "(${asset}-128)/2")
    list(GET models ${course} model)
    string(REPEAT "${model}," 10 grid)
    string(APPEND grid "${model}")
    set(log "${EVIDENCE}/squaldon-${mode}-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_squaldon.ini"
        --set stop.timer=120 --set "race.course=${course}" --set "race.grid=${grid}"
        --set "diagnostics.log=${log}" --set "modern.assets=${ASSET_SOURCE}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Squaldon bank ${asset} race failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    # Special-class starters use activeFlag 0. Only -1 means inactive.
    # Track placement must handle all four starters and skip the empty slots.
    foreach(index RANGE 0 3)
        if(NOT trace MATCHES "scenario-start rival=${index} [^\n]*model=${model} active=0")
            message(FATAL_ERROR "Special-class starter ${index} was not placed in bank ${asset}")
        endif()
    endforeach()
    if(trace MATCHES "scenario-start rival=([4-9]|10) ")
        message(FATAL_ERROR "Special-class placement touched an inactive slot")
    endif()
    foreach(required "scenario-start rival=0 [^\n]*model=${model}"
            "authored Squaldon player body installed asset=72"
            "authored Squaldon rival body installed asset=${asset}"
            "authored Vainqure rival body installed asset=${asset}"
            "authored Bulshade rival body installed asset=${asset}")
        if(NOT trace MATCHES "${required}")
            message(FATAL_ERROR "Squaldon bank ${asset} missed ${required}")
        endif()
    endforeach()
endforeach()
