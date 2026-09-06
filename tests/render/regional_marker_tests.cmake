# Real disc images are external fixtures, never bundled with the tests.
# Dedicated variables prevent one installed disc from satisfying all regions.
foreach(case valid wrong_count missing_menu failed)
    add_test(NAME route_completion_${case} COMMAND ${CMAKE_COMMAND}
        -DCASE=${case} -P ${CMAKE_CURRENT_LIST_DIR}/route_completion_tests.cmake)
    set_tests_properties(route_completion_${case} PROPERTIES LABELS unit)
    if(NOT case STREQUAL "valid")
        set_tests_properties(route_completion_${case} PROPERTIES WILL_FAIL TRUE)
    endif()
endforeach()
if(TARGET rage-racer-smoke AND UNIX AND NOT APPLE)
    foreach(region PAL NTSC-U NTSC-J)
        string(REPLACE "-" "_" region_key "${region}")
        string(TOLOWER "${region_key}" region_name)
        add_test(NAME modern_region_${region_name} COMMAND ${CMAKE_COMMAND}
            -DGAME=$<TARGET_FILE:rage-racer-smoke>
            -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
            -DDISC_ENV=RAGE_PORT_${region_key}_CUE -DEXPECT_REGION=${region}
            -DFPS=logic -DMARKER_FRAME=1000
            -P ${CMAKE_CURRENT_LIST_DIR}/verify_sampled_vram_marker.cmake)
        set_tests_properties(modern_region_${region_name} PROPERTIES
            LABELS "e2e;regional" TIMEOUT 180
            SKIP_REGULAR_EXPRESSION "SKIP: no disc")
        if(TARGET rage-racer)
            add_test(NAME modern_repeat_${region_name} COMMAND ${CMAKE_COMMAND}
                -DGAME=$<TARGET_FILE:rage-racer>
                -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
                -DDISC_ENV=RAGE_PORT_${region_key}_CUE -DEXPECT_REGION=${region}
                -P ${CMAKE_CURRENT_LIST_DIR}/verify_regional_races.cmake)
            set_tests_properties(modern_repeat_${region_name} PROPERTIES
                LABELS "e2e;regional;endurance" TIMEOUT 660
                SKIP_REGULAR_EXPRESSION "SKIP: no disc")
        endif()
    endforeach()
endif()
