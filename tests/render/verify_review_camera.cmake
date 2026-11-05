cmake_minimum_required(VERSION 3.20)
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/review-camera-${id}")
file(MAKE_DIRECTORY "${root}")
foreach(case baseline disabled missing invalid active)
    set(options --set diagnostics.review_camera=true
        --set diagnostics.review_target_x=27499 --set diagnostics.review_target_y=880
        --set diagnostics.review_azimuth=180 --set diagnostics.review_elevation=0)
    if(case STREQUAL baseline)
        set(options --set diagnostics.review_camera=false)
    elseif(case STREQUAL disabled)
        list(APPEND options --set diagnostics.review_camera=false)
    endif()
    if(NOT case STREQUAL missing)
        list(APPEND options --set diagnostics.review_target_z=-33296)
    endif()
    if(case STREQUAL invalid)
        list(APPEND options --set diagnostics.review_distance=0)
    else()
        list(APPEND options --set diagnostics.review_distance=900)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env PSYZ_HEADLESS=1
        SDL_AUDIODRIVER=dummy SDL_MAC_BACKGROUND_APP=1 "${GAME}"
        --config "${SOURCE}/rage-port.ini" --scenario "${SOURCE}/race-scenario.ini"
        --set modern.assets=disc --set modern.authored_cars=0
        --set mods.directory= --set "hd.store=${root}/hd"
        --set video.renderer=modern --set video.internal_scale=2 --set video.aspect=16:9
        --set video.fps=logic --set mods.random_time_of_day=false
        --set race.car=3 --set race.course=1 --set race.class=0
        --set start.player_track_point=316 --set start.camera=1
        --set start.freeze=true --set input.disable_host=true --set sync.random=11@0=12345
        --set run.frames=1800 --set stop.scene=12 --set stop.timer=400
        --set report.camera_state=true --set diagnostics.test_log=true
        --set diagnostics.material_usage=true --set diagnostics.material_usage_start_frame=350
        --set "diagnostics.log=${root}/${case}.log"
        --set "diagnostics.modern_dump=${root}/${case}.ppm"
        --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=380
        --set diagnostics.modern_dump_offscreen=true ${options}
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 30 RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Review camera scenario failed: ${case} ${root}\n${output}${error}")
    endif()
    file(READ "${root}/${case}.log" log)
    string(REGEX MATCH "camera: pos=[^\n]+ angle=[^ ]+" gameCamera "${output}")
    if(NOT gameCamera)
        message(FATAL_ERROR "Missing original game camera report: ${root}")
    endif()
    file(SHA256 "${root}/${case}.ppm" hash)
    if(case STREQUAL baseline)
        set(baselineHash "${hash}")
        set(baselineCamera "${gameCamera}")
    elseif(NOT gameCamera STREQUAL baselineCamera)
        message(FATAL_ERROR "Review view changed game camera/physics state: ${case} ${root}")
    endif()
    if(case STREQUAL active)
        if(hash STREQUAL baselineHash)
            message(FATAL_ERROR "Review camera did not change the rendered view: ${root}")
        endif()
        string(REGEX MATCHALL "review-camera: [^\n]+" records "${log}")
        list(LENGTH records count)
        if(NOT count EQUAL 1 OR NOT log MATCHES "\"position\":\\[27499.00,880.00,-34196.00\\]")
            message(FATAL_ERROR "Incorrect or repeated review pose: ${root}")
        endif()
        if(NOT log MATCHES "native-material-use: [^\n]*\"view\":\"main\"[^\n]*\"material\":147,\"variant\":2")
            message(FATAL_ERROR "Target roof material was not submitted: ${root}")
        endif()
    else()
        if(NOT hash STREQUAL baselineHash OR log MATCHES "review-camera:")
            message(FATAL_ERROR "Disabled/invalid review camera changed normal rendering: ${case} ${root}")
        endif()
        if((case STREQUAL missing OR case STREQUAL invalid) AND NOT log MATCHES "review camera ignored:")
            message(FATAL_ERROR "Invalid configuration was not diagnosed: ${case} ${root}")
        endif()
    endif()
endforeach()
