string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(work "${OUTPUT_ROOT}/mirrored-track-${id}")
file(MAKE_DIRECTORY "${work}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
    "${SOURCE}/assets" "${work}/assets" RESULT_VARIABLE link_result)
if(NOT link_result EQUAL 0)
    message(FATAL_ERROR "Could not link test assets: ${work}")
endif()
file(WRITE "${work}/rage-port.cfg" "renderer=modern\nmodern.draw_distance=2\n")

function(run_mirror name mirrored)
    set(mirror_env --unset=RAGE_PORT_SMOKE_MIRROR_TRACK)
    if(mirrored)
        list(APPEND mirror_env RAGE_PORT_SMOKE_MIRROR_TRACK=1)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        SDL_AUDIODRIVER=dummy RAGE_PORT_SCENARIO=1 RAGE_PORT_SMOKE_FRAMES=2450
        RAGE_PORT_INPUT_SCRIPT=2250-2450:LEFT+X RAGE_PORT_SCENE_TRACE=1
        ${mirror_env} "${GAME}"
        WORKING_DIRECTORY "${work}" TIMEOUT 165 RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${name} exited ${result}: ${work}\n${log}")
    endif()
    string(REGEX MATCH "steer=(-?[0-9]+) course_mirror=([0-9]+)" steering "${log}")
    if(NOT steering)
        message(FATAL_ERROR "${name} omitted steering diagnostics: ${work}\n${log}")
    endif()
    set("${name}_steer" "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set("${name}_log" "${log}" PARENT_SCOPE)
endfunction()

run_mirror(normal FALSE)
run_mirror(mirror TRUE)
if(normal_steer EQUAL 0 OR mirror_steer EQUAL 0 OR
   (normal_steer GREATER 0 AND mirror_steer LESS 0) OR
   (normal_steer LESS 0 AND mirror_steer GREATER 0))
    message(FATAL_ERROR "Mirrored steering reversed: normal=${normal_steer} mirror=${mirror_steer}: ${work}")
endif()
if(NOT mirror_log MATCHES "scene-frame.* scene=12 .*faces=[1-9][0-9]*")
    message(FATAL_ERROR "Mirrored course lost main-view 3D faces: ${work}")
endif()
if(mirror_log MATCHES "ERROR:|runtime error:")
    message(FATAL_ERROR "Mirrored course emitted a runtime error: ${work}\n${mirror_log}")
endif()
