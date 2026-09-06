add_executable(material_transaction_tests material_transaction_tests.c)
target_link_libraries(material_transaction_tests PRIVATE rage-render-world)
if(MSVC)
    target_compile_options(material_transaction_tests PRIVATE /W4 /WX /UNDEBUG)
else()
    target_compile_options(material_transaction_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
endif()
add_test(NAME material_transaction COMMAND material_transaction_tests)
set_tests_properties(material_transaction PROPERTIES LABELS unit)

add_executable(disc_stream_table_tests
    disc_stream_table_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/disc_iso.c
    ${CMAKE_SOURCE_DIR}/src/port/disc_stream_table.c
)
target_include_directories(disc_stream_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port
)
if(MSVC)
    target_compile_options(disc_stream_table_tests PRIVATE /W4 /WX)
    target_compile_definitions(disc_stream_table_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(disc_stream_table_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME disc_stream_table COMMAND disc_stream_table_tests)
target_link_libraries(disc_stream_table_tests PRIVATE rage-content)
set_tests_properties(disc_stream_table PROPERTIES LABELS functional)

add_executable(stream_table_disc_tests
    stream_table_disc_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/disc_cue.c
    ${CMAKE_SOURCE_DIR}/src/port/disc_iso.c
    ${CMAKE_SOURCE_DIR}/src/port/disc_stream_table.c
)
target_include_directories(stream_table_disc_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port
)
if(MSVC)
    target_compile_options(stream_table_disc_tests PRIVATE /W4 /WX)
    target_compile_definitions(stream_table_disc_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(stream_table_disc_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME stream_table COMMAND stream_table_disc_tests ${CMAKE_SOURCE_DIR})
target_link_libraries(stream_table_disc_tests PRIVATE rage-content)
# Skips rather than fails when there is no disc to derive the table from.
set_tests_properties(stream_table PROPERTIES LABELS functional
    SKIP_RETURN_CODE 77 TIMEOUT 120)
add_executable(mod_manifest_tests mod_manifest_tests.c)
add_executable(mod_package_tests mod_package_tests.c)
target_link_libraries(mod_package_tests PRIVATE rage-mod-package)
add_test(NAME mod_package COMMAND mod_package_tests)
set_tests_properties(mod_package PROPERTIES LABELS unit)
target_link_libraries(mod_manifest_tests PRIVATE rage-rmesh)
if(MSVC)
    target_compile_options(mod_manifest_tests PRIVATE /W4 /WX)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mod_manifest_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME mod_manifest COMMAND mod_manifest_tests)

set_tests_properties(mod_manifest PROPERTIES LABELS functional)
add_executable(mod_provider_tests mod_provider_tests.c)
add_executable(mod_selection_tests mod_selection_tests.c)
add_executable(mod_provider_stdin_tests mod_provider_stdin_tests.c)
add_executable(mod_file_snapshot_tests mod_file_snapshot_tests.c)
add_executable(mod_file_policy_tests mod_file_policy_tests.c)
target_link_libraries(mod_file_policy_tests PRIVATE rage-rmesh)
add_test(NAME mod_file_policy COMMAND mod_file_policy_tests)
set_tests_properties(mod_file_policy PROPERTIES LABELS unit)
target_link_libraries(mod_file_snapshot_tests PRIVATE rage-rmesh)
add_test(NAME mod_file_snapshot COMMAND mod_file_snapshot_tests)
set_tests_properties(mod_file_snapshot PROPERTIES LABELS unit)
target_link_libraries(mod_provider_stdin_tests PRIVATE rage-rmesh)
add_test(NAME mod_provider_stdin COMMAND mod_provider_stdin_tests)
set_tests_properties(mod_provider_stdin PROPERTIES LABELS unit)
target_link_libraries(mod_selection_tests PRIVATE rage-rmesh)
add_test(NAME mod_selection COMMAND mod_selection_tests)
set_tests_properties(mod_selection PROPERTIES LABELS unit)
target_link_libraries(mod_provider_tests PRIVATE rage-rmesh)
if(MSVC)
    target_compile_options(mod_provider_tests PRIVATE /W4 /WX)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mod_provider_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME mod_provider_selection COMMAND mod_provider_tests)
set_tests_properties(mod_provider_selection PROPERTIES LABELS unit)
add_executable(mod_asset_fallback_tests
    mod_asset_fallback_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/mod_assets.c)
target_link_libraries(mod_asset_fallback_tests PRIVATE rage-rmesh)
target_include_directories(mod_asset_fallback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port)
if(MSVC)
    target_compile_options(mod_asset_fallback_tests PRIVATE /W4 /WX)
    target_compile_definitions(mod_asset_fallback_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mod_asset_fallback_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME mod_asset_fallback COMMAND mod_asset_fallback_tests)
foreach(mode valid invalid future semantic unreadable requires)
    add_test(NAME mod_provider_${mode} COMMAND mod_asset_fallback_tests ${mode})
    set_tests_properties(mod_provider_${mode} PROPERTIES LABELS functional)
endforeach()

set_tests_properties(mod_asset_fallback PROPERTIES LABELS functional)
add_executable(release_package_tests release_package_tests.c)
if(MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(release_package_tests PRIVATE
        /clang:-Wall /clang:-Wextra /clang:-Werror
        /clang:-Wno-unsafe-buffer-usage)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(release_package_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME release_package COMMAND release_package_tests ${CMAKE_SOURCE_DIR})

set_tests_properties(release_package PROPERTIES LABELS functional)
if(TARGET rage-extract AND TARGET rage-pack)
    add_executable(mod_archive_tests mod_archive_tests.c)
    add_dependencies(mod_archive_tests rage-extract rage-pack)
    target_link_libraries(mod_archive_tests PRIVATE SDL3::SDL3)
    if(MSVC)
        target_compile_options(mod_archive_tests PRIVATE /W4 /WX /UNDEBUG)
        target_compile_definitions(mod_archive_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(mod_archive_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME mod_archive COMMAND mod_archive_tests $<TARGET_FILE:rage-extract> $<TARGET_FILE:rage-pack>)
    set_tests_properties(mod_archive PROPERTIES LABELS functional TIMEOUT 180)
    add_executable(texture_patch_tests texture_patch_tests.c ${CMAKE_SOURCE_DIR}/src/port/texture_patch.c)
    target_include_directories(texture_patch_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port)
    target_link_libraries(texture_patch_tests PRIVATE miniz rage-json)
    if(MSVC)
        target_compile_options(texture_patch_tests PRIVATE /W4 /WX /UNDEBUG)
        target_compile_definitions(texture_patch_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(texture_patch_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME texture_patch COMMAND texture_patch_tests)
    set_tests_properties(texture_patch PROPERTIES LABELS functional)
endif()

if(TARGET rage-racer-smoke)
    if(TARGET rage-pcm-check)
        add_test(NAME class_award_fmv COMMAND ${CMAKE_COMMAND}
            -DGAME=$<TARGET_FILE:rage-racer-smoke>
            -DPCM_CHECK=$<TARGET_FILE:rage-pcm-check>
            -DPACING_CHECK=$<TARGET_FILE:rage-fmv-pacing-check>
            -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/class_award_fmv.cmake)
        set_tests_properties(class_award_fmv PROPERTIES LABELS e2e
            TIMEOUT 150 SKIP_REGULAR_EXPRESSION "SKIP:")
        foreach(region NTSC-U NTSC-J)
            string(REPLACE "-" "_" key "${region}")
            string(TOLOWER "${key}" suffix)
            add_test(NAME class_award_fmv_${suffix} COMMAND ${CMAKE_COMMAND}
                -DGAME=$<TARGET_FILE:rage-racer-smoke>
                -DPCM_CHECK=$<TARGET_FILE:rage-pcm-check>
                -DPACING_CHECK=$<TARGET_FILE:rage-fmv-pacing-check>
                -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
                -DDISC_ENV=RAGE_PORT_${key}_CUE -DEXPECT_REGION=${region}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/class_award_fmv.cmake)
            set_tests_properties(class_award_fmv_${suffix} PROPERTIES
                LABELS "e2e;regional" TIMEOUT 150 SKIP_REGULAR_EXPRESSION "SKIP:")
        endforeach()
        foreach(region PAL NTSC-U NTSC-J)
            string(REPLACE "-" "_" key "${region}")
            string(TOLOWER "${key}" suffix)
            foreach(series 0 1)
                foreach(class 0 1 2 3)
                    if(series EQUAL 0 AND class EQUAL 0)
                        continue() # Registered above.
                    endif()
                    set(test class_award_fmv_${suffix}_${series}_${class})
                    add_test(NAME ${test} COMMAND ${CMAKE_COMMAND}
                        -DGAME=$<TARGET_FILE:rage-racer-smoke>
                        -DPCM_CHECK=$<TARGET_FILE:rage-pcm-check>
                        -DPACING_CHECK=$<TARGET_FILE:rage-fmv-pacing-check>
                        -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
                        -DDISC_ENV=RAGE_PORT_${key}_CUE -DEXPECT_REGION=${region}
                        -DCLASS=${class} -DSERIES=${series}
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/class_award_fmv.cmake)
                    set_tests_properties(${test} PROPERTIES LABELS "e2e;regional"
                        TIMEOUT 150 SKIP_REGULAR_EXPRESSION "SKIP:")
                endforeach()
            endforeach()
        endforeach()
        add_test(NAME modern_fmv_audio COMMAND ${CMAKE_COMMAND}
            -DGAME=$<TARGET_FILE:rage-racer-smoke>
            -DPCM_CHECK=$<TARGET_FILE:rage-pcm-check>
            -DPACING_CHECK=$<TARGET_FILE:rage-fmv-pacing-check>
            -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/modern_fmv_audio.cmake)
        set_tests_properties(modern_fmv_audio PROPERTIES LABELS e2e
            TIMEOUT 600 SKIP_REGULAR_EXPRESSION "SKIP: no disc image")
        foreach(stream 0 1 2 3 4 6 7 8 9 10)
            add_test(NAME modern_fmv_audio_${stream} COMMAND ${CMAKE_COMMAND}
                -DGAME=$<TARGET_FILE:rage-racer-smoke>
                -DPCM_CHECK=$<TARGET_FILE:rage-pcm-check>
                -DPACING_CHECK=$<TARGET_FILE:rage-fmv-pacing-check>
                -DSOURCE=${CMAKE_SOURCE_DIR} -DOUTPUT_ROOT=${CMAKE_BINARY_DIR}
                -DSTREAM=${stream}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/modern_fmv_audio.cmake)
            set_tests_properties(modern_fmv_audio_${stream} PROPERTIES LABELS e2e
                TIMEOUT 600 SKIP_REGULAR_EXPRESSION "SKIP: no disc image")
        endforeach()
    endif()
    add_test(
        NAME fmv_frame_layout
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_frame.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(fmv_frame_layout PROPERTIES LABELS e2e)
    # Skips rather than fails when there is no disc to read the movies
    # or the music from; see tests/support/disc_image.py.
    set_tests_properties(fmv_frame_layout PROPERTIES TIMEOUT 75
        SKIP_RETURN_CODE 77)
    add_test(
        NAME fmv_without_ffmpeg
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_without_ffmpeg.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(fmv_without_ffmpeg PROPERTIES LABELS e2e)
    set_tests_properties(fmv_without_ffmpeg PROPERTIES TIMEOUT 90
        SKIP_RETURN_CODE 77)
    add_test(
        NAME fmv_pacing
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_pacing.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(fmv_pacing PROPERTIES LABELS e2e)
    set_tests_properties(fmv_pacing PROPERTIES TIMEOUT 600
        SKIP_RETURN_CODE 77)
    add_test(
        NAME fmv_all_audio
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_fmv_pacing.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
            --all-audio
    )
    set_tests_properties(fmv_all_audio PROPERTIES LABELS e2e TIMEOUT 720
        SKIP_RETURN_CODE 77)

    add_test(
        NAME modern_disc_import_bootstrap
        COMMAND $<TARGET_FILE:rage-racer-smoke>
            --set video.renderer=modern
            --set modern.assets=disc
            --set run.frames=1
    )
set_tests_properties(modern_disc_import_bootstrap PROPERTIES LABELS e2e)
    set_tests_properties(modern_disc_import_bootstrap PROPERTIES
        TIMEOUT 20
        ENVIRONMENT "SDL_AUDIODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "native assets generated by C importer")
endif()
