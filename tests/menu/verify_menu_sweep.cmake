if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

function(run_sweep variable pattern expected input_script)
    set(scene_trace)
    if(variable STREQUAL "RAGE_PORT_SMOKE_OPTION_SWEEP")
        set(scene_trace RAGE_PORT_SCENE_TRACE=1)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
        RAGE_PORT_SMOKE_FRAMES=2200 "${variable}=1" ${scene_trace}
        "RAGE_PORT_INPUT_SCRIPT=${input_script}" "${GAME}"
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 135 RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${variable} failed (${result}):\n${log}")
    endif()
    string(REGEX MATCHALL "${pattern}" entries "${log}")
    set(visited)
    foreach(entry IN LISTS entries)
        string(REGEX REPLACE ".*=([0-9]+)" "\\1" value "${entry}")
        list(APPEND visited "${value}")
    endforeach()
    list(REMOVE_DUPLICATES visited)
    list(SORT visited)
    if(NOT "${visited}" STREQUAL "${expected}")
        message(FATAL_ERROR "${variable} visited ${visited}; expected ${expected}")
    endif()
    if(variable STREQUAL "RAGE_PORT_SMOKE_OPTION_SWEEP" AND
       NOT log MATCHES "scene-frame .* scene=23 .*draws=[1-9][0-9]* .*faces=[1-9][0-9]*")
        message(FATAL_ERROR "OPTION controller screens emitted no 3D model faces")
    endif()
    foreach(failure "primitive buffer exhausted" "ERROR: AddressSanitizer" "runtime error:")
        string(FIND "${log}" "${failure}" found)
        if(NOT found EQUAL -1)
            message(FATAL_ERROR "${variable} reported ${failure}")
        endif()
    endforeach()
endfunction()

# CMake sorts strings lexically, so 10–12 follow 1 in the expected lists.
run_sweep(RAGE_PORT_SMOKE_MENU_SWEEP "menu sweep [^\n]*screen=[0-9]+"
    "1;10;11;12;2;3;4;5;6;7;8;9" "400:START,500:DOWN,520:CROSS")
run_sweep(RAGE_PORT_SMOKE_OPTION_SWEEP "option sweep [^\n]*mode=[0-9]+"
    "1;10;11;2;3;4;5;6;7;8;9" "400:START,500:UP,520:CROSS")
message(STATUS "Rendered all 12 frontend screens and all 11 OPTION modes")
