# Real disc images are external fixtures, never bundled with the tests.
# Dedicated variables prevent one installed disc from satisfying all regions.
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
    endforeach()
endif()
