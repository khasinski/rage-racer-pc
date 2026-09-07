string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef id)
set(root "${OUTPUT_ROOT}/native-world-${id}")
file(MAKE_DIRECTORY "${root}/mod/textures")
execute_process(COMMAND "${FIXTURE}" "${root}" RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Native fixture generation failed: ${root}")
endif()
file(RENAME "${root}/terrain.png" "${root}/mod/textures/terrain.png")
file(WRITE "${root}/mod/mod.toml" "[mod]\nid = \"native-world-test\"\n[textures]\n\"track.big1.terrain.material.0\" = \"textures/terrain.png\"\n")
file(WRITE "${root}/scenario.ini" "[video]\nrenderer=modern\n[race]\nenabled=true\nmode=grand-prix\nclass=0\ncourse=0\ncar=3\n[run]\nframes=900\n[stop]\nscene=12\ntimer=20\n")
function(run name)
    set(correction_env)
    if(name STREQUAL "geometry-512" OR name STREQUAL "correction-reference")
        list(APPEND correction_env RAGE_GPU_GP0_TRACE_SCENE=12 RAGE_GPU_GP0_TRACE_TIMER=430
            "RAGE_GPU_GP0_TRACE_VRAM=${root}/${name}.vram" PSYZ_VRAM_BATCH_TRACE=1)
    endif()
    if(name STREQUAL "correction-reference")
        list(APPEND correction_env PSYZ_REFERENCE_CORRECTION_PIXELS=1
            PSYZ_REFERENCE_FULL_VRAM_BATCH=1)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_AUDIODRIVER=dummy
        --unset=PSYZ_REFERENCE_CORRECTION_PIXELS --unset=PSYZ_REFERENCE_FULL_VRAM_BATCH ${correction_env}
        "RAGE_PORT_MODERN_ASSETS=${root}" RAGE_PORT_MODERN_ASSET_TRACE=1
        RAGE_VERIFY_CAPTURE_PUBLICATION=1
        RAGE_VERIFY_WORLD_PUBLICATION=1
        "RAGE_PORT_MODS_DIRECTORY=${root}/mod" "${GAME}" ${ARGN}
        WORKING_DIRECTORY "${SOURCE}" TIMEOUT 105 RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    set(log "${output}${error}")
    file(WRITE "${root}/${name}.log" "${log}")
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "${name} failed (${result}): ${root}")
    endif()
    set(log "${log}" PARENT_SCOPE)
endfunction()
function(require pattern)
    if(NOT log MATCHES "${pattern}")
        message(FATAL_ERROR "Missing ${pattern}: ${root}")
    endif()
endfunction()
run(race --scenario "${root}/scenario.ini" --set diagnostics.performance_trace=true)
require("native-geometry-prepare frame=[0-9]+ instances=[1-9][0-9]* main_vertices=[1-9][0-9]* mirror_vertices=[0-9]+ snapshot_ms=[0-9]+[.][0-9]+ warm_ms=[0-9]+[.][0-9]+ shadow_setup_ms=[0-9]+[.][0-9]+ main_ms=[0-9]+[.][0-9]+ mirror_ms=[0-9]+[.][0-9]+ completeness_ms=[0-9]+[.][0-9]+")
require("native GPU pipeline ready")
require("semantic asset mod native-world-test")
require("native texture override track[.]big1[.]terrain[.]material[.]0 <- textures/terrain[.]png \\(64x32\\)")
require("native car paint asset=")
require("native world frame=[0-9]+ camera=[1-9][0-9]* instances=[1-9][0-9]* cached=[1-9][0-9]* textures=[0-9]+ vertices=[1-9][0-9]* spans=[1-9][0-9]*")
require("native draws frame=[0-9]+ draws=[1-9][0-9]* vertices=[1-9][0-9]*")
run(benchmark --scenario "${root}/scenario.ini"
    --set diagnostics.performance_trace=false --set diagnostics.modern_asset_trace=false
    --set "diagnostics.modern_dump=${root}/benchmark.ppm"
    --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=10
    --set diagnostics.modern_dump_scene=true
    --set diagnostics.modern_prepare_repeat=3)
require("native-live-prepare-benchmark repeats=3 instances=[1-9][0-9]* p50_ms=[0-9]+[.][0-9]+ p95_ms=[0-9]+[.][0-9]+ max_ms=[0-9]+[.][0-9]+")
require("scene=12 timer=20")
file(SHA256 "${root}/benchmark.ppm.draws.txt" before_prepare)
file(SHA256 "${root}/benchmark.ppm.restored.draws.txt" after_prepare)
if(NOT before_prepare STREQUAL after_prepare)
    message(FATAL_ERROR "Live benchmark changed prepared draw state: ${root}")
endif()
foreach(limit 512 0 1)
    run(geometry-${limit} --scenario "${root}/scenario.ini"
        --set video.fps=logic --set video.internal_scale=1 --set start.freeze=true
        --set stop.timer=431 --set run.frames=1400
        --set diagnostics.modern_cpu_geometry=false
        --set "diagnostics.modern_geometry_limit=${limit}"
        --set diagnostics.performance_trace=true
        --set "diagnostics.modern_dump=${root}/geometry-${limit}.ppm"
        --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=430
        --set diagnostics.modern_dump_scene=true)
    require("scene=12 timer=431")
    require("native draws frame=[0-9]+ draws=[1-9][0-9]* vertices=[1-9][0-9]* view=mirror")
    if(limit EQUAL 512)
        require("capture-publication verify=match frame=[0-9]+")
        require("world-publication verify=match frame=[0-9]+")
        require("vram-batch-sync copy=0 bytes=0")
        require("resident_draws=[1-9][0-9]* local_fallbacks=0")
        string(REGEX MATCHALL "native-world-upload [^\r\n]+" world_uploads "${log}")
        if(NOT world_uploads)
            message(FATAL_ERROR "No resident terrain uploads: ${root}")
        endif()
        list(GET world_uploads -1 last_world_upload)
        string(REGEX REPLACE ".*indices=([0-9]+).*" "\\1" world_reset_limit "${last_world_upload}")
    elseif(limit EQUAL 0)
        require("resident_draws=0 local_fallbacks=[1-9][0-9]*")
        if(log MATCHES "native-resident-upload")
            message(FATAL_ERROR "Zero geometry budget created a resident buffer: ${root}")
        endif()
    else()
        require("resident_draws=[1-9][0-9]* local_fallbacks=[1-9][0-9]*")
    endif()
    foreach(suffix .ppm .ppm.draws.txt)
        file(SHA256 "${root}/geometry-${limit}${suffix}" actual)
        if(limit EQUAL 512)
            set("reference${suffix}" "${actual}")
        elseif(NOT actual STREQUAL "${reference${suffix}}")
            message(FATAL_ERROR "Geometry budget ${limit} changed ${suffix}: ${root}")
        endif()
    endforeach()
endforeach()
run(correction-reference --scenario "${root}/scenario.ini"
    --set video.fps=logic --set video.internal_scale=1 --set start.freeze=true
    --set stop.timer=431 --set run.frames=1400
    --set diagnostics.modern_cpu_geometry=false --set diagnostics.modern_geometry_limit=512
    --set "diagnostics.modern_dump=${root}/correction-reference.ppm"
    --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=430
    --set diagnostics.modern_dump_scene=true)
require("scene=12 timer=431")
foreach(suffix .vram .ppm .ppm.draws.txt)
    file(SHA256 "${root}/correction-reference${suffix}" reference_pixels)
    file(SHA256 "${root}/geometry-512${suffix}" run_pixels)
    if(NOT reference_pixels STREQUAL run_pixels)
        message(FATAL_ERROR "Correction runs changed ${suffix}: ${root}")
    endif()
endforeach()
run(unbatched --scenario "${root}/scenario.ini"
    --set video.fps=logic --set video.internal_scale=1 --set start.freeze=true
    --set stop.timer=431 --set run.frames=1400
    --set diagnostics.modern_cpu_geometry=false --set diagnostics.modern_geometry_limit=512
    --set diagnostics.modern_unbatched_draws=true
    --set "diagnostics.modern_dump=${root}/unbatched.ppm"
    --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=430
    --set diagnostics.modern_dump_scene=true)
require("scene=12 timer=431")
foreach(suffix .ppm .ppm.draws.txt)
    file(SHA256 "${root}/unbatched${suffix}" actual)
    if(NOT actual STREQUAL "${reference${suffix}}")
        message(FATAL_ERROR "Draw batching changed ${suffix}: ${root}")
    endif()
endforeach()
foreach(limit 0 1 ${world_reset_limit})
    run(world-${limit} --scenario "${root}/scenario.ini"
        --set video.fps=logic --set video.internal_scale=1 --set start.freeze=true
        --set stop.timer=431 --set run.frames=1400
        --set diagnostics.modern_cpu_geometry=false --set diagnostics.modern_geometry_limit=512
        --set "diagnostics.modern_world_vertex_limit=${limit}" --set diagnostics.performance_trace=true
        --set "diagnostics.modern_dump=${root}/world-${limit}.ppm"
        --set diagnostics.modern_dump_scene_id=12 --set diagnostics.modern_dump_timer=430
        --set diagnostics.modern_dump_scene=true)
    require("scene=12 timer=431")
    if(limit EQUAL world_reset_limit)
        string(REGEX MATCHALL "native-world-upload [^\r\n]+reset=1[^\r\n]*" resets "${log}")
        list(LENGTH resets reset_count)
        if(reset_count LESS 10)
            message(FATAL_ERROR "Terrain budget did not exercise repeated rebuilds: ${root}")
        endif()
    elseif(log MATCHES "native-world-upload")
        message(FATAL_ERROR "Insufficient terrain budget unexpectedly created a packed draw: ${root}")
    endif()
    foreach(suffix .ppm .ppm.draws.txt)
        file(SHA256 "${root}/world-${limit}${suffix}" actual)
        if(NOT actual STREQUAL "${reference${suffix}}")
            message(FATAL_ERROR "Terrain budget ${limit} changed ${suffix}: ${root}")
        endif()
    endforeach()
endforeach()
run(attract --set video.renderer=modern --set race.enabled=false --set boot.direct=false
    --set run.frames=2500 --set stop.scene=30 --set stop.timer=200)
require("scene=30 timer=200")
require("native shadow map frame=[0-9]+ draws=[1-9][0-9]* masked=[1-9][0-9]*")
message(STATUS "Native world compiled fixture passed: ${root}")
