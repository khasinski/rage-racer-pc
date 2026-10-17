# Keep coverage for the legacy Asset Browser when its optional interpreter is
# available. Windows' Store alias is an executable, not a Python interpreter.
find_program(RAGE_LEGACY_PYTHON NAMES python3 python)
if(RAGE_LEGACY_PYTHON)
    execute_process(COMMAND "${RAGE_LEGACY_PYTHON}" -c "import sys; sys.exit(0)"
        RESULT_VARIABLE legacy_python_result OUTPUT_QUIET ERROR_QUIET TIMEOUT 10)
endif()
foreach(kind gltf sky)
    if(RAGE_LEGACY_PYTHON AND legacy_python_result STREQUAL "0")
        add_test(NAME asset_${kind}_export COMMAND "${RAGE_LEGACY_PYTHON}"
            ${CMAKE_SOURCE_DIR}/tools/assetbrowser/test_${kind}.py)
    else()
        add_test(NAME asset_${kind}_export COMMAND ${CMAKE_COMMAND} -E echo
            "SKIP: legacy Asset Browser interpreter unavailable")
    endif()
    set_tests_properties(asset_${kind}_export PROPERTIES LABELS tooling
        SKIP_REGULAR_EXPRESSION "SKIP: legacy Asset Browser interpreter unavailable")
endforeach()

if(TARGET rage-gte-replay)
    add_test(NAME gte_replay_usage COMMAND rage-gte-replay)
set_tests_properties(gte_replay_usage PROPERTIES LABELS functional)
    set_tests_properties(gte_replay_usage PROPERTIES WILL_FAIL TRUE)
endif()
add_executable(mesh_obj_tests mesh_obj_tests.c)
target_link_libraries(mesh_obj_tests PRIVATE rage-rmesh)
add_test(NAME mesh_obj COMMAND mesh_obj_tests)
set_tests_properties(mesh_obj PROPERTIES LABELS tooling)
