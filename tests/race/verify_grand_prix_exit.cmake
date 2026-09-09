if(NOT DEFINED GAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "GAME and SOURCE are required")
endif()

file(READ "${SOURCE}/include/game/menu.h" menu_header)
file(READ "${SOURCE}/src/port/native_game_state.c" native_state)
string(FIND "${menu_header}" "RAGE_NATIVE_UI_SCRIPT(CourseSelectSavePromptBanner, 2);" menu_script)
string(FIND "${native_state}" "NATIVE_UI_SCRIPT(g_NativeCourseSelectSavePromptBanner, 0x800827fcu)" native_script)
if(menu_script EQUAL -1 OR native_script EQUAL -1)
    message(FATAL_ERROR "save-prompt banner is not decoded as a native UI script")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        SDL_AUDIODRIVER=dummy
        RAGE_PORT_SMOKE_FRAMES=2200
        RAGE_PORT_DISABLE_HOST_INPUT=1
        RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,1015:CROSS,1100:DOWN,1106:DOWN,1112:CROSS,1180:CROSS
        "${GAME}"
    WORKING_DIRECTORY "${SOURCE}"
    TIMEOUT 70
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Grand Prix exit run failed (${result}):\n${log}")
endif()
if(NOT log MATCHES "smoke state frame=.* scene=(2|4|24) ")
    message(FATAL_ERROR "Grand Prix exit did not leave course selection:\n${log}")
endif()
if(log MATCHES "AddressSanitizer" OR log MATCHES "runtime error")
    message(FATAL_ERROR "Grand Prix exit reported a memory error:\n${log}")
endif()
message(STATUS "End Grand Prix left course selection without overrunning its UI script")
