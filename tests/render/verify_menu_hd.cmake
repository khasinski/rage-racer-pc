string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/menu-hd-${id}")
file(MAKE_DIRECTORY "${root}/mods")
execute_process(COMMAND "${FIXTURE}" write "${root}/mods/hd.png" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot create HD menu fixture")
endif()
file(WRITE "${root}/mods/mod.toml" "[textures]\n")
foreach(item grand-prix time-attack custom load-save options)
    file(APPEND "${root}/mods/mod.toml" "\"ui.title.${item}\" = \"hd.png\"\n\"ui.title.${item}.selected\" = \"hd.png\"\n")
endforeach()
file(WRITE "${root}/game.ini" "[modern]\nassets = disc\n[video]\nrenderer = modern\ninternal_scale = 4\naspect = 4:3\nfps = logic\npost = none\ngrading = off\n[mods]\ndirectory = ${root}/mods\n[input]\ndisable_host = true\n[run]\nframes = 620\n[diagnostics]\nmodern_dump = ${root}/menu.ppm\nmodern_dump_frame = 580\nmodern_dump_offscreen = true\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
    RAGE_PORT_DISABLE_HOST_INPUT=1 "${GAME}" --config "${root}/game.ini"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60 RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "HD menu runtime failed: ${root}\n${output}${error}")
endif()
# A legacy pack can still be installed, but the native menu must render
# identically to a clean run. Compare at the same scene/frame, not wall time.
execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
    "${GAME}" --config "${root}/game.ini" --set "mods.directory=${root}/empty"
    --set "diagnostics.modern_dump=${root}/baseline.ppm"
    WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60 RESULT_VARIABLE baseline_result)
if(NOT baseline_result EQUAL 0)
    message(FATAL_ERROR "Native menu baseline failed: ${root}")
endif()
file(SHA256 "${root}/menu.ppm" with_legacy)
file(SHA256 "${root}/baseline.ppm" without_legacy)
if(NOT with_legacy STREQUAL without_legacy)
    message(FATAL_ERROR "Retired title overrides altered the native menu: ${root}")
endif()
execute_process(COMMAND "${FIXTURE}" check
 "${root}/menu.ppm" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "HD menu pixels failed: ${root}\n${output}${error}")
endif()
