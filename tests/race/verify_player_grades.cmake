file(MAKE_DIRECTORY "${EVIDENCE}")
if(NOT DEFINED ASSET_SOURCE)
    set(ASSET_SOURCE disc)
endif()
if(ASSET_SOURCE STREQUAL "disc")
    set(mode disc)
else()
    set(mode cache)
endif()
set(cases
    "0,1,12,Erriso"
    "0,2,14,Erriso"
    "0,3,16,Erriso"
    "1,1,20,Abeille"
    "1,2,22,Abeille"
    "2,1,26,Pegase"
    "3,1,30,Esperanza"
    "3,2,32,Esperanza"
    "3,3,34,Esperanza"
    "3,4,36,Esperanza"
    "4,1,40,Acceron"
    "4,2,42,Acceron"
    "4,3,44,Acceron"
    "5,1,48,Bayonet"
    "5,2,50,Bayonet"
    "6,1,54,Hijack"
    "7,1,58,Fatalita"
    "7,2,60,Fatalita"
    "8,1,64,Istante")
foreach(case IN LISTS cases)
    string(REPLACE "," ";" fields "${case}")
    list(GET fields 0 car)
    list(GET fields 1 variant)
    list(GET fields 2 asset)
    list(GET fields 3 name)
    set(log "${EVIDENCE}/player-grade-${mode}-bank-${asset}.log")
    file(REMOVE "${log}")
    execute_process(COMMAND "${GAME}" --scenario "${SOURCE}/tests/scenarios/authored_compact_later.ini"
        --set stop.timer=120 --set "race.car=${car}" --set "race.variant=${variant}"
        --set "modern.assets=${ASSET_SOURCE}" --set "diagnostics.log=${log}"
        WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Player grade ${case} failed: ${status}\n${output}\n${errors}")
    endif()
    file(READ "${log}" trace)
    if(NOT trace MATCHES "authored ${name} player body installed asset=${asset} ")
        message(FATAL_ERROR "Player grade ${case} did not install its authored body")
    endif()
    if(trace MATCHES "ignoring invalid race.variant")
        message(FATAL_ERROR "Player grade ${case} was rejected")
    endif()
endforeach()
