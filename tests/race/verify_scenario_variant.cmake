file(MAKE_DIRECTORY "${EVIDENCE}")
# Last valid variants and the next invalid index exercise catalog boundaries.
set(cases "0,3,16,valid,true" "3,4,36,valid,true" "8,1,64,valid,true"
    "12,0,72,valid,true" "0,4,10,invalid,true" "12,1,72,invalid,true"
    "0,1,12,valid,false")
foreach(case IN LISTS cases)
    string(REPLACE "," ";" fields "${case}")
    list(GET fields 0 car)
    list(GET fields 1 variant)
    list(GET fields 2 asset)
    list(GET fields 3 expected)
    list(GET fields 4 direct)
    set(log "${EVIDENCE}/scenario-variant-${car}-${variant}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_compact_later.ini"
        --set stop.timer=120 --set "race.car=${car}" --set "race.variant=${variant}"
        --set "boot.direct=${direct}"
        --set "diagnostics.log=${log}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Variant ${case} race failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    if(NOT trace MATCHES "imported native mesh asset=${asset} set=0 ")
        message(FATAL_ERROR "Variant ${case} did not import the expected player bank")
    endif()
    if(expected STREQUAL "invalid")
        if(NOT trace MATCHES "ignoring invalid race.variant=${variant}")
            message(FATAL_ERROR "Variant ${case} was not rejected")
        endif()
    elseif(trace MATCHES "ignoring invalid race.variant")
        message(FATAL_ERROR "Valid variant ${case} was rejected")
    endif()
endforeach()
