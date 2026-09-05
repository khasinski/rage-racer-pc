# Linux debug-only harness, using the existing CMake toolchain (no interpreter).
cmake_minimum_required(VERSION 3.20)
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This isolated XDG harness currently supports Linux only")
endif()
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
foreach(required GAME DISC)
    if(NOT DEFINED ${required} OR NOT EXISTS "${${required}}")
        message(FATAL_ERROR "Pass -D${required}=an-existing-absolute-path")
    endif()
    get_filename_component(${required} "${${required}}" ABSOLUTE)
endforeach()
if(NOT DEFINED CONFIG)
    set(CONFIG "${ROOT}/rage-port.ini")
endif()
if(NOT EXISTS "${CONFIG}")
    message(FATAL_ERROR "CONFIG must name an existing renderer configuration")
endif()
get_filename_component(CONFIG "${CONFIG}" ABSOLUTE)
file(SHA256 "${GAME}" gameSha256)
foreach(pair RUNS:2 RACES:0 LAPS:3 SPEED:6000 COURSE:0 CLASS:0 SERIES:0 MARKER_FRAME:900 MARKER_EVERY:900 MARKER_LIMIT:4 MAX_FRAMES:30000 TIMEOUT:900)
    string(REPLACE ":" ";" fields "${pair}")
    list(GET fields 0 key)
    list(GET fields 1 default)
    if(NOT DEFINED ${key})
        set(${key} ${default})
    endif()
    if(NOT "${${key}}" MATCHES "^[0-9]+$")
        message(FATAL_ERROR "${key} must be an unsigned integer")
    endif()
endforeach()
if(RUNS LESS 1 OR RUNS GREATER 20 OR RACES GREATER 100 OR LAPS LESS 1 OR LAPS GREATER 100 OR
   SPEED LESS 1 OR SPEED GREATER 100000 OR COURSE GREATER 3 OR CLASS GREATER 5 OR SERIES GREATER 1 OR
   MARKER_LIMIT LESS 1 OR MARKER_LIMIT GREATER 100 OR TIMEOUT LESS 1 OR
   MAX_FRAMES LESS 1 OR MAX_FRAMES GREATER 1000000 OR
   MARKER_FRAME GREATER 2147483647 OR MARKER_EVERY GREATER 2147483647)
    message(FATAL_ERROR "Debug drive limits out of range")
endif()
if(NOT DEFINED OUTPUT)
    set(OUTPUT "${ROOT}/build/debug-drive")
endif()
get_filename_component(OUTPUT "${OUTPUT}" ABSOLUTE)
string(TIMESTAMP stamp "%Y%m%d-%H%M%S")
string(RANDOM LENGTH 6 ALPHABET 0123456789abcdef suffix)
set(session "${OUTPUT}/${stamp}-${suffix}")
file(MAKE_DIRECTORY "${session}")
set(toolEnv)
if(DEFINED TOOLS_ROOT)
    get_filename_component(TOOLS_ROOT "${TOOLS_ROOT}" ABSOLUTE)
    set(manifest "${TOOLS_ROOT}/usr/share/vulkan/implicit_layer.d/renderdoc_capture.json")
    if(NOT EXISTS "${manifest}")
        message(FATAL_ERROR "TOOLS_ROOT must contain the extracted RenderDoc package")
    endif()
    file(READ "${manifest}" json)
    string(JSON json SET "${json}" layer library_path
        "\"${TOOLS_ROOT}/usr/lib64/renderdoc/librenderdoc.so\"")
    file(MAKE_DIRECTORY "${session}/implicit-layers")
    file(WRITE "${session}/implicit-layers/renderdoc_capture.json" "${json}")
    list(APPEND toolEnv "VK_ADD_IMPLICIT_LAYER_PATH=${session}/implicit-layers"
        "VK_ADD_LAYER_PATH=${TOOLS_ROOT}/usr/share/vulkan/explicit_layer.d"
        "LD_LIBRARY_PATH=${TOOLS_ROOT}/usr/lib64:$ENV{LD_LIBRARY_PATH}")
    if(NOT DEFINED RENDERDOC)
        set(RENDERDOC "${TOOLS_ROOT}/usr/bin/renderdoccmd")
    endif()
endif()
set(launcher)
set(gpu false)
if(DEFINED RENDERDOC)
    if(NOT EXISTS "${RENDERDOC}")
        message(FATAL_ERROR "RENDERDOC must name renderdoccmd")
    endif()
    get_filename_component(RENDERDOC "${RENDERDOC}" ABSOLUTE)
    set(launcher "${RENDERDOC}" capture -w -d "${ROOT}")
    set(gpu true)
endif()
set(validationEnv)
if(VALIDATION)
    # Requires the Khronos layer installed or supplied via VK_LAYER_PATH and
    # LD_LIBRARY_PATH. Loader diagnostics and validation stdout are preserved.
    list(APPEND validationEnv "VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation"
        "VK_VALIDATION_VALIDATE_SYNC=1" "VK_KHRONOS_VALIDATION_VALIDATE_SYNC=1"
        "VK_LOADER_DEBUG=error,warn,layer")
endif()
message(STATUS "Debug drive evidence: ${session}")
foreach(run RANGE 1 ${RUNS})
    set(dir "${session}/run-${run}")
    file(MAKE_DIRECTORY "${dir}/config" "${dir}/state")
    configure_file("${CONFIG}" "${dir}/renderer.ini" COPYONLY)
    set(args --config "${dir}/renderer.ini" --scenario "${ROOT}/race-scenario.ini"
        --set "disc.image=${DISC}" --set "race.course=${COURSE}" --set "race.class=${CLASS}" --set "race.series=${SERIES}"
        --set race.after_finish=repeat --set autopilot.enabled=true
        --set "autopilot.laps=${LAPS}" --set "autopilot.speed=${SPEED}"
        --set "autopilot.races=${RACES}"
        --set "autopilot.max_frames=${MAX_FRAMES}" --set video.renderer=modern
        --set start.freeze=false --set "diagnostics.log=${dir}/game.log"
        --set diagnostics.marker_capture=true --set "diagnostics.marker_frame=${MARKER_FRAME}"
        --set "diagnostics.marker_every=${MARKER_EVERY}" --set "diagnostics.marker_limit=${MARKER_LIMIT}"
        --set "diagnostics.renderdoc=${gpu}" --set diagnostics.renderdoc_burst=2
        --set "diagnostics.renderdoc_limit=${MARKER_LIMIT}")
    file(WRITE "${dir}/settings.txt" "game=${GAME}\ngame_sha256=${gameSha256}\nconfig_source=${CONFIG}\ndisc=${DISC}\nargs=${args}\nvalidation=${VALIDATION}\nrenderdoc=${RENDERDOC}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        "XDG_CONFIG_HOME=${dir}/config" "XDG_STATE_HOME=${dir}/state"
        "SDL_AUDIODRIVER=dummy" ${toolEnv} ${validationEnv} ${launcher} "${GAME}" ${args}
        WORKING_DIRECTORY "${ROOT}" TIMEOUT ${TIMEOUT} RESULT_VARIABLE result
        OUTPUT_FILE "${dir}/launcher.log" ERROR_FILE "${dir}/launcher-errors.log")
    set(log "")
    if(EXISTS "${dir}/game.log")
        file(READ "${dir}/game.log" log)
    endif()
    set(completion "laps=${LAPS}")
    if(RACES GREATER 0)
        set(completion "races=${RACES}")
    endif()
    if(NOT result EQUAL 0 OR NOT log MATCHES "autopilot result=complete ${completion}(\r?\n|$)" OR
       log MATCHES "autopilot result=failed|No supported SDL_GPU backend|SDL_CreateGPUDevice:")
        message(FATAL_ERROR "Run ${run} incomplete (result=${result}); inspect ${dir}")
    endif()
    file(GLOB captures "${dir}/state/rage-racer/*.rdc")
    file(GLOB markers "${dir}/state/rage-racer/markers/*-info.txt")
    list(LENGTH captures captureCount)
    list(LENGTH markers markerCount)
    if(NOT markerCount GREATER 0 OR (gpu AND NOT captureCount GREATER 0))
        message(FATAL_ERROR "Run ${run} finished but requested evidence is missing: ${dir}")
    endif()
    if(VALIDATION)
        file(READ "${dir}/launcher.log" stdout)
        file(READ "${dir}/launcher-errors.log" stderr)
        string(APPEND log "\n${stdout}\n${stderr}")
        if(NOT log MATCHES "Insert instance layer \"VK_LAYER_KHRONOS_validation\"")
            message(FATAL_ERROR "Validation activation unconfirmed: ${dir}")
        endif()
        if(log MATCHES "Validation Error|SYNC-HAZARD|Failed to load|cannot open shared object")
            message(FATAL_ERROR "Validation findings or layer loading failure: ${dir}")
        endif()
    endif()
    file(WRITE "${dir}/result.txt" "drive=complete\n${completion}\nmarker_files=${markerCount}\ngpu_captures=${captureCount}\nvisual_correctness=not_automatically_asserted\n")
    message(STATUS "Run ${run}: ${completion}, ${markerCount} markers, ${captureCount} GPU captures")
endforeach()
