if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

foreach(fast IN ITEMS false true)
    # Use the real pad buffer to release buttons between injected presses.
    # Square is the retail back button; Circle also confirms in these menus.
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy RAGE_PORT_SMOKE_FRAMES=2200
        "${GAME}" --set "mods.fast_menus=${fast}"
        --set report.camera_state=true --set input.raw_script=0:UP
        --set "input.state_script=4@40:DOWN,4@44:CONFIRM,8@200@1:CONFIRM,8@450@4:DOWN,8@454@4:CONFIRM,8@650@5:CONFIRM,8@900@5:CONFIRM,8@1200@5:SQUARE,8@1500@4:SQUARE"
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 90 RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Fast menus=${fast} navigation failed (${result}):\n${log}")
    endif()
    if(NOT log MATCHES "menu=1 busy=0" OR
       NOT log MATCHES "smoke state input [^\n]*timer=1500 buttons=0080")
        message(FATAL_ERROR "Fast menus=${fast} did not return from customization via car selection:\n${log}")
    endif()
    foreach(screen IN ITEMS 1 4 5)
        if(NOT log MATCHES "smoke menu frame=[^\n]*screen=${screen}\n")
            message(FATAL_ERROR "Fast menus=${fast} never reached screen ${screen}:\n${log}")
        endif()
    endforeach()
    foreach(failure IN ITEMS "primitive buffer exhausted" "ERROR: AddressSanitizer" "runtime error:")
        string(FIND "${log}" "${failure}" found)
        if(NOT found EQUAL -1)
            message(FATAL_ERROR "Fast menus=${fast} reported ${failure}:\n${log}")
        endif()
    endforeach()
endforeach()
message(STATUS "Course -> car -> customize tires -> car -> course works at both speeds")
