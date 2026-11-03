string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/ui-atlas-${id}")
file(MAKE_DIRECTORY "${root}/mods")
set(writeMode write-atlas)
set(regionSuffix "")
if(REGION)
 set(writeMode write)
 set(regionSuffix ".region.112.160.112.16")
endif()
execute_process(COMMAND "${FIXTURE}" "${writeMode}" "${root}/mods/atlas.png" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
 message(FATAL_ERROR "Cannot write UI atlas fixture")
endif()
# The Press Start sprite in scene 4 uses page 25, CLUT 0x7e84. This fixture
# supplies no ui.title.* cell, proving ordinary scene-scoped sprites reach HD.
file(WRITE "${root}/mods/mod.toml" "[textures]\n\"ui.scene.4.page.25.clut.32388${regionSuffix}\" = \"atlas.png\"\n")
file(WRITE "${root}/game.ini" "[modern]\nassets = disc\n[video]\nrenderer = modern\ninternal_scale = 4\naspect = 4:3\nfps = logic\npost = none\ngrading = off\n[mods]\ndirectory = ${root}/mods\n[run]\nframes = 395\n[diagnostics]\nmodern_dump = ${root}/ui.ppm\nmodern_dump_frame = 380\nmodern_dump_offscreen = true\nui_trace = true\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
 RAGE_PORT_DISABLE_HOST_INPUT=1 "${GAME}" --config "${root}/game.ini"
 WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60 RESULT_VARIABLE result
 OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
 message(FATAL_ERROR "UI atlas game failed: ${root}\n${output}${error}")
endif()
execute_process(COMMAND "${FIXTURE}" check-atlas "${root}/ui.ppm" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
 message(FATAL_ERROR "Scene-scoped HD atlas detail failed: ${root}\n${output}${error}")
endif()
