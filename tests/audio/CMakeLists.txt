if(RAGE_RUBY_EXECUTABLE)
add_test(
    NAME sequence_state_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_sequence_state_layout.rb
        ${CMAKE_SOURCE_DIR}
)

set_tests_properties(sequence_state_layout PROPERTIES LABELS functional)
add_test(
    NAME audio_state_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_audio_state_layout.rb
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/sound_runtime.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/loaded_audio_voices.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_audio.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/update_car_drivetrain.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/records.c
        ${CMAKE_SOURCE_DIR}/include/game/track.h
        ${RAGE_HOST_STATE_SOURCES}
)

set_tests_properties(audio_state_layout PROPERTIES LABELS functional)
endif()

add_executable(cd_audio_policy_tests cd_audio_policy_tests.c)
target_include_directories(cd_audio_policy_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(cd_audio_policy_tests PRIVATE
        -Wall -Wextra -Werror)
endif()
add_test(NAME cd_audio_policy COMMAND cd_audio_policy_tests)
set_tests_properties(cd_audio_policy PROPERTIES LABELS functional)

add_executable(audio_parameter_table_tests
    audio_parameter_table_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/audio_parameter_table.c)
target_include_directories(audio_parameter_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(audio_parameter_table_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(audio_parameter_table_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(audio_parameter_table_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(audio_parameter_table_tests PRIVATE psyz)
add_test(NAME audio_parameter_table COMMAND audio_parameter_table_tests)
set_tests_properties(audio_parameter_table PROPERTIES LABELS unit)

add_executable(interpolate_audio_parameter_tests
    interpolate_audio_parameter_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/interpolate_audio_parameter.c)
target_include_directories(interpolate_audio_parameter_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(interpolate_audio_parameter_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(interpolate_audio_parameter_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(interpolate_audio_parameter_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(interpolate_audio_parameter_tests PRIVATE psyz)
add_test(NAME interpolate_audio_parameter COMMAND interpolate_audio_parameter_tests)
set_tests_properties(interpolate_audio_parameter PROPERTIES LABELS unit)

add_executable(current_sequence_audio_tests
    current_sequence_audio_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/apply_current_sequence_audio.c)
target_include_directories(current_sequence_audio_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include)
target_compile_definitions(current_sequence_audio_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(current_sequence_audio_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(current_sequence_audio_tests PRIVATE psyz)
add_test(NAME current_sequence_audio COMMAND current_sequence_audio_tests)
set_tests_properties(current_sequence_audio PROPERTIES LABELS unit)

if(TARGET rage-racer-smoke)
    add_test(
        NAME audio_output
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_audio_output.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(audio_output PROPERTIES LABELS e2e)
    set_tests_properties(audio_output PROPERTIES TIMEOUT 90)

    add_test(
        NAME menu_music_tempo
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_menu_music_tempo.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(menu_music_tempo PROPERTIES LABELS e2e)
    set_tests_properties(menu_music_tempo PROPERTIES TIMEOUT 180)

    add_test(
        NAME race_cdda
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_race_cdda.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(race_cdda PROPERTIES LABELS e2e)
    set_tests_properties(race_cdda PROPERTIES TIMEOUT 135
        SKIP_RETURN_CODE 77)
    add_test(
        NAME engine_audio
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_engine_audio.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(engine_audio PROPERTIES LABELS e2e)
    set_tests_properties(engine_audio PROPERTIES TIMEOUT 210)
endif()
