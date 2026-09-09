if(NOT DEFINED GAME OR NOT DEFINED CHECK OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT_ROOT)
    message(FATAL_ERROR "GAME, CHECK, SOURCE and OUTPUT_ROOT are required")
endif()
set(capture_dir "${OUTPUT_ROOT}/mirror-entry")
file(REMOVE_RECURSE "${capture_dir}")
file(MAKE_DIRECTORY "${capture_dir}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        SDL_AUDIODRIVER=dummy
        RAGE_PORT_SMOKE_FRAMES=1800
        RAGE_PORT_DISABLE_HOST_INPUT=1
        RAGE_PORT_INPUT_SCRIPT=400:START,500:START,650:CROSS,1015:CROSS,1165:CROSS,1265:CROSS,1533-10000:CROSS
        RAGE_PORT_SMOKE_CAPTURE_DIR=${capture_dir}
        RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE=1
        RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN=365
        RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX=371
        RAGE_PORT_SMOKE_CAPTURE_SCENE=12
        RAGE_PORT_SMOKE_STOP_SCENE=12
        RAGE_PORT_SMOKE_STOP_SCENE_TIMER=371
        "${GAME}"
    WORKING_DIRECTORY "${SOURCE}"
    TIMEOUT 120
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "mirror-entry route failed (${result}):\n${output}${error}")
endif()
execute_process(COMMAND "${CHECK}" "${capture_dir}" RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "mirror-entry validation failed:\n${output}${error}")
endif()
message(STATUS "Mirror entry retained the main scene through every zero-height clip frame")
