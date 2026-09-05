set(sandbox "${EVIDENCE}/transition-test")
file(MAKE_DIRECTORY "${sandbox}/bu00")
file(COPY "${SMOKE}" DESTINATION "${sandbox}")
get_filename_component(name "${SMOKE}" NAME)
execute_process(COMMAND "${sandbox}/${name}" --scenario "${SOURCE}/race-scenario.ini"
    --set race.class=1 --set race.car=0 --set race.grid=2,2,2,2,2,2,2,2,2,2,2
    --set race.after_finish=repeat --set run.frames=3500
    --set hooks.finish_frame=1000 --set hooks.auto_confirm_frame=1300
    --set modern.assets=disc --set video.renderer=modern
    WORKING_DIRECTORY "${SOURCE}" RESULT_VARIABLE status
    OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 180)
set(trace "${output}\n${errors}")
file(WRITE "${EVIDENCE}/authored-transition.log" "${trace}")
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Authored car race transition failed: ${status}")
endif()
string(REGEX MATCHALL "smoke state frame=[0-9]+ scene=12" starts "${trace}")
list(LENGTH starts count)
if(count LESS 2)
    message(FATAL_ERROR "Did not enter a second race with the existing model cache")
endif()
foreach(required "authored Erriso player body installed asset=10"
        "authored Erriso rival body installed asset=96"
        "scenario race finished after_finish=repeat")
    if(NOT trace MATCHES "${required}")
        message(FATAL_ERROR "Transition missed ${required}")
    endif()
endforeach()
