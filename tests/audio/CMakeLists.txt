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
