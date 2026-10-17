# Normal menu input: choose CUSTOM, then the first rival model, race and replay.
set(disc "$ENV{RAGE_PORT_DISC_IMAGE}")
if(NOT EXISTS "${disc}")
    message("SKIP: custom replay requires RAGE_PORT_DISC_IMAGE")
    return()
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/custom-replay-${id}")
file(MAKE_DIRECTORY "${root}/bu00")
file(WRITE "${root}/bu00/.fixture" "private test state")
get_filename_component(game_name "${GAME}" NAME)
get_filename_component(game_directory "${GAME}" DIRECTORY)
file(COPY "${GAME}" DESTINATION "${root}")
file(GLOB libraries "${game_directory}/*.dll" "${game_directory}/*.dylib")
if(libraries)
    file(COPY ${libraries} DESTINATION "${root}")
endif()
set(input "400:START,500:DOWN,520:DOWN,540:CROSS,900:CROSS")
foreach(index RANGE 0 9)
    math(EXPR frame "1100 + ${index} * 150")
    # A range feeds the held-button path used by the car carousel.
    string(APPEND input ",${frame}-${frame}:RIGHT")
endforeach()
string(APPEND input ",2700:CROSS,2900:CROSS,3100:CROSS,3200-6400:CROSS")
foreach(renderer classic modern)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
        "${root}/${game_name}" --config "${SOURCE}/rage-port.ini"
        --set "disc.image=${disc}" --set disc.choose=false
        --set "mods.directory=${root}/mods" --set "cars.profile=${SOURCE}/cars.toml"
        --set "diagnostics.log=${root}/${renderer}-runtime.log"
        --set "video.renderer=${renderer}" --set video.internal_scale=1
        --set video.aspect=4:3 --set input.disable_host=true
        --set "input.script=${input}" --set run.frames=6500
        --set hooks.finish_frame=4500 --set stop.scene=17 --set stop.timer=250
        WORKING_DIRECTORY "${root}" TIMEOUT 120 RESULT_VARIABLE result
        OUTPUT_FILE "${root}/${renderer}.log" ERROR_FILE "${root}/${renderer}.log")
    file(READ "${root}/${renderer}.log" log)
    if(NOT result STREQUAL "0" OR NOT log MATCHES "scene 17" OR
       NOT log MATCHES "session=1 model=13")
        message(FATAL_ERROR "Custom rival replay failed (${renderer}): ${root}")
    endif()
    if(log MATCHES "AddressSanitizer|runtime error:|primitive buffer exhausted")
        message(FATAL_ERROR "Custom replay memory/render failure: ${root}")
    endif()
endforeach()
message(STATUS "Custom rival model reached replay in both renderers: ${root}")
