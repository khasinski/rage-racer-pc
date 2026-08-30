add_test(NAME asset_gltf_export
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/assetbrowser/test_gltf.py)
set_tests_properties(asset_gltf_export PROPERTIES LABELS tooling)
add_test(NAME asset_sky_export
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/assetbrowser/test_sky.py)
set_tests_properties(asset_sky_export PROPERTIES LABELS tooling)

if(RAGE_RUBY_EXECUTABLE)
add_test(
    NAME visual_batch_refinement
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_batch_refinement.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_batch.rb
)

set_tests_properties(visual_batch_refinement PROPERTIES LABELS tooling)
add_test(
    NAME visual_run
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_run.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_run.rb
)

set_tests_properties(visual_run PROPERTIES LABELS tooling)
add_test(
    NAME visual_budget
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_budget.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_budget.rb
)

set_tests_properties(visual_budget PROPERTIES LABELS tooling)
add_test(
    NAME visual_triage
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_visual_triage.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_visual_triage.rb
)

set_tests_properties(visual_triage PROPERTIES LABELS tooling)
add_test(
    NAME gpu_digest_compare
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_gpu_digest_compare.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_gpu_digest_compare.rb
)

set_tests_properties(gpu_digest_compare PROPERTIES LABELS tooling)
add_test(
    NAME gp0_compare
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_compare.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_gp0_compare.rb
)

set_tests_properties(gp0_compare PROPERTIES LABELS tooling)
add_test(
    NAME gp0_bundle
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_bundle.rb
        ${CMAKE_SOURCE_DIR}/tools/rage_gp0_bundle.rb
)
set_tests_properties(gp0_bundle PROPERTIES LABELS tooling)
add_test(
    NAME gp0_replay
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_replay.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_gp0_replay.c
)
set_tests_properties(gp0_replay PROPERTIES LABELS tooling)
add_test(
    NAME gp0_raster_compare
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_gp0_raster_compare.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_gp0_raster_compare.rb
)
set_tests_properties(gp0_raster_compare PROPERTIES LABELS tooling)
add_test(
    NAME vram_delta
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_vram_delta.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_vram_delta.rb
)
set_tests_properties(vram_delta PROPERTIES LABELS tooling)
add_test(
    NAME visible_cells_compare
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_visible_cells_compare.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_visible_cells_compare.rb
)

set_tests_properties(visible_cells_compare PROPERTIES LABELS tooling)
add_test(
    NAME input_replay
    COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/verify_input_replay.rb
            ${CMAKE_SOURCE_DIR}/tools/rage_input_replay.rb
)
set_tests_properties(input_replay PROPERTIES LABELS tooling)
endif()

if(TARGET rage-gte-replay)
    add_test(NAME gte_replay_usage COMMAND rage-gte-replay)
set_tests_properties(gte_replay_usage PROPERTIES LABELS functional)
    set_tests_properties(gte_replay_usage PROPERTIES WILL_FAIL TRUE)
endif()
