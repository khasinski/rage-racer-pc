# Each bank contains the same rival body but has an independent runtime cache.
file(MAKE_DIRECTORY "${EVIDENCE}")
foreach(asset RANGE 102 110 2)
    if(asset EQUAL 102)
        set(class 1)
        set(course 3)
    else()
        set(class 2)
        math(EXPR course "(${asset}-104)/2")
    endif()
    if(course EQUAL 3)
        set(model 3)
    else()
        math(EXPR model "2-${course}")
    endif()
    string(REPEAT "${model}," 10 grid)
    string(APPEND grid "${model}")
    set(log "${EVIDENCE}/abeille-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_abeille.ini"
        --set stop.timer=120 --set "race.class=${class}" --set "race.course=${course}"
        --set "race.grid=${grid}" --set "diagnostics.log=${log}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Abeille bank ${asset} race failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    foreach(required "scenario-start rival=0 [^\n]*model=${model}"
            "authored Abeille player body installed asset=18"
            "authored Abeille rival body installed asset=${asset}")
        if(NOT trace MATCHES "${required}")
            message(FATAL_ERROR "Abeille bank ${asset} missed ${required}")
        endif()
    endforeach()
endforeach()
