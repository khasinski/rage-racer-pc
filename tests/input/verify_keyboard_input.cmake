execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    SDL_AUDIODRIVER=dummy RAGE_PORT_SMOKE_FRAMES=30
    RAGE_PORT_TEST_KEY_TAP=Return RAGE_PORT_INPUT_DEBUG=1
    "${GAME}" WORKING_DIRECTORY "${SOURCE}" TIMEOUT 45
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}${error}")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Keyboard-input run failed (${result}):\n${log}")
endif()
foreach(expected IN ITEMS "keyboard pad mask=0800"
                          "type=41 held=0800 pressed=0800")
    string(FIND "${log}" "${expected}" position)
    if(position LESS 0)
        message(FATAL_ERROR "Missing '${expected}':\n${log}")
    endif()
endforeach()
message(STATUS "Short SDL Return tap produced the game's PAD_START rising edge")
