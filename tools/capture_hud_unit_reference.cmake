# Developer reference capture, not a player-facing import requirement.
foreach(required GAME SOURCE DISC EXPECT_REGION OUTPUT_ROOT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing -D${required}=...")
    endif()
endforeach()
if(NOT EXPECT_REGION MATCHES "^(PAL|NTSC-U)$")
    message(FATAL_ERROR "Reference region must be PAL or NTSC-U")
endif()
set(root "${OUTPUT_ROOT}/${EXPECT_REGION}")
file(MAKE_DIRECTORY "${root}")
foreach(car RANGE 0 12)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
        "${GAME}" --scenario "${SOURCE}/race-scenario.ini"
        --set "disc.image=${DISC}" --set race.mode=time-attack
        --set "race.car=${car}" --set race.class=0 --set race.course=0
        --set poc.speed_units=auto --set input.disable_host=true
        --set start.freeze=false --set start.random_seed=1
        --set input.state_script= --set mods.random_time_of_day=false
        --set "mods.directory=${root}/no-mods" --set "hd.store=${root}/no-hd"
        --set video.renderer=classic --set video.fps=logic
        --set run.frames=1600 --set stop.scene=12 --set stop.timer=180
        --set report.hud_layout=true --set "dump.vram=${root}/car-${car}.vram"
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 60
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    file(WRITE "${root}/car-${car}.log" "${log}")
    if(NOT result EQUAL 0 OR NOT EXISTS "${root}/car-${car}.vram")
        message(FATAL_ERROR "Reference capture failed: ${EXPECT_REGION} car ${car}")
    endif()
    if(NOT log MATCHES "region=${EXPECT_REGION}[^A-Za-z-]" OR
       NOT log MATCHES "hud-layout: .\"model\":${car},\"page\":10,\"clut\":13224,\"rect\":\\[0,112,96,94\\]")
        message(FATAL_ERROR "Unexpected disc identity or HUD layout: ${root}/car-${car}.log")
    endif()
    message(STATUS "Reference ${EXPECT_REGION} car ${car}")
endforeach()
