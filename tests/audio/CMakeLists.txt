if(RAGE_RUBY_EXECUTABLE)
add_test(
    NAME audio_state_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_audio_state_layout.rb
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/sound_runtime.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/audio_initialization.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/reset_audio_voice_state.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_audio.c
        ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_pause_request.c
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

add_executable(cd_pause_request_tests
    cd_pause_request_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_pause_request.c)
target_include_directories(cd_pause_request_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(cd_pause_request_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(cd_pause_request_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(cd_pause_request_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(cd_pause_request_tests PRIVATE psyz)
add_test(NAME cd_pause_request COMMAND cd_pause_request_tests)
set_tests_properties(cd_pause_request PROPERTIES LABELS functional)

add_executable(cd_audio_control_tests
    cd_audio_control_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_audio_control.c)
target_include_directories(cd_audio_control_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(cd_audio_control_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(cd_audio_control_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(cd_audio_control_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME cd_audio_control COMMAND cd_audio_control_tests)
set_tests_properties(cd_audio_control PROPERTIES LABELS unit)

add_executable(cd_track_requests_tests
    cd_track_requests_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_track_requests.c)
target_include_directories(cd_track_requests_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(cd_track_requests_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(cd_track_requests_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(cd_track_requests_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(cd_track_requests_tests PRIVATE psyz)
add_test(NAME cd_track_requests COMMAND cd_track_requests_tests)
set_tests_properties(cd_track_requests PROPERTIES LABELS unit)

add_executable(cd_audio_runtime_tests
    cd_audio_runtime_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_audio.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_audio_control.c)
target_include_directories(cd_audio_runtime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(cd_audio_runtime_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(cd_audio_runtime_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(cd_audio_runtime_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(cd_audio_runtime_tests PRIVATE psyz)
add_test(NAME cd_audio_runtime COMMAND cd_audio_runtime_tests)
set_tests_properties(cd_audio_runtime PROPERTIES LABELS unit)

add_executable(cd_track_table_tests
    cd_track_table_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/build_cd_track_table.c)
target_include_directories(cd_track_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(cd_track_table_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(cd_track_table_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(cd_track_table_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME cd_track_table COMMAND cd_track_table_tests)
set_tests_properties(cd_track_table PROPERTIES LABELS unit)

add_executable(cd_volume_fade_tests
    cd_volume_fade_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_volume_fade.c)
target_include_directories(cd_volume_fade_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(cd_volume_fade_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(cd_volume_fade_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(cd_volume_fade_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME cd_volume_fade COMMAND cd_volume_fade_tests)
set_tests_properties(cd_volume_fade PROPERTIES LABELS unit)

add_executable(cd_volume_tests
    cd_volume_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/cd/cd_volume.c)
target_include_directories(cd_volume_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(cd_volume_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(cd_volume_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(cd_volume_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME cd_volume COMMAND cd_volume_tests)
set_tests_properties(cd_volume PROPERTIES LABELS unit)

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

add_executable(sequence_playback_tests
    sequence_playback_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/sequence_playback.c)
target_include_directories(sequence_playback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(sequence_playback_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(sequence_playback_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(sequence_playback_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(sequence_playback_tests PRIVATE psyz)
add_test(NAME sequence_playback COMMAND sequence_playback_tests)
set_tests_properties(sequence_playback PROPERTIES LABELS unit)

add_executable(audio_runtime_control_tests
    audio_runtime_control_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/audio_runtime_control.c)
target_include_directories(audio_runtime_control_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(audio_runtime_control_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(audio_runtime_control_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(audio_runtime_control_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME audio_runtime_control COMMAND audio_runtime_control_tests)
set_tests_properties(audio_runtime_control PROPERTIES LABELS unit)

add_executable(sequence_volume_tests
    sequence_volume_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/sequence_volume.c)
target_include_directories(sequence_volume_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(sequence_volume_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(sequence_volume_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(sequence_volume_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME sequence_volume COMMAND sequence_volume_tests)
set_tests_properties(sequence_volume PROPERTIES LABELS unit)

add_executable(sound_runtime_tests
    sound_runtime_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/sound_runtime.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/reset_audio_voice_state.c)
target_include_directories(sound_runtime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(sound_runtime_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(sound_runtime_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(sound_runtime_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME sound_runtime COMMAND sound_runtime_tests)
set_tests_properties(sound_runtime PROPERTIES LABELS unit)

add_executable(audio_initialization_tests
    audio_initialization_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/audio_initialization.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/reset_audio_voice_state.c)
target_include_directories(audio_initialization_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(audio_initialization_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(audio_initialization_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(audio_initialization_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME audio_initialization COMMAND audio_initialization_tests)
set_tests_properties(audio_initialization PROPERTIES LABELS unit)

add_executable(vab_sequence_slots_tests
    vab_sequence_slots_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/vab_sequence_slots.c)
target_include_directories(vab_sequence_slots_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(vab_sequence_slots_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(vab_sequence_slots_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(vab_sequence_slots_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME vab_sequence_slots COMMAND vab_sequence_slots_tests)
set_tests_properties(vab_sequence_slots PROPERTIES LABELS unit)

add_executable(pitched_sound_cue_tests
    pitched_sound_cue_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/pitched_sound_cue.c)
target_include_directories(pitched_sound_cue_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(pitched_sound_cue_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(pitched_sound_cue_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(pitched_sound_cue_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(pitched_sound_cue_tests PRIVATE psyz)
add_test(NAME pitched_sound_cue COMMAND pitched_sound_cue_tests)
set_tests_properties(pitched_sound_cue PROPERTIES LABELS unit)

add_executable(stereo_sound_cue_tests
    stereo_sound_cue_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/stereo_sound_cue.c)
target_include_directories(stereo_sound_cue_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(stereo_sound_cue_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(stereo_sound_cue_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(stereo_sound_cue_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(stereo_sound_cue_tests PRIVATE psyz)
add_test(NAME stereo_sound_cue COMMAND stereo_sound_cue_tests)
set_tests_properties(stereo_sound_cue PROPERTIES LABELS unit)

add_executable(engine_sound_runtime_tests
    engine_sound_runtime_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/engine_sound_runtime.c)
target_include_directories(engine_sound_runtime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(engine_sound_runtime_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(engine_sound_runtime_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(engine_sound_runtime_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(engine_sound_runtime_tests PRIVATE psyz)
add_test(NAME engine_sound_runtime COMMAND engine_sound_runtime_tests)
set_tests_properties(engine_sound_runtime PROPERTIES LABELS unit)

add_executable(audio_slot_loading_tests
    audio_slot_loading_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/audio_slot_loading.c)
target_include_directories(audio_slot_loading_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(audio_slot_loading_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(audio_slot_loading_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(audio_slot_loading_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(audio_slot_loading_tests PRIVATE psyz)
add_test(NAME audio_slot_loading COMMAND audio_slot_loading_tests)
set_tests_properties(audio_slot_loading PROPERTIES LABELS unit)

add_executable(update_effect_voice_states_tests
    update_effect_voice_states_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/update_effect_voice_states.c)
target_include_directories(update_effect_voice_states_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(update_effect_voice_states_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(update_effect_voice_states_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(update_effect_voice_states_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(update_effect_voice_states_tests PRIVATE psyz)
add_test(NAME update_effect_voice_states COMMAND update_effect_voice_states_tests)
set_tests_properties(update_effect_voice_states PROPERTIES LABELS unit)

add_executable(effect_voice_runtime_tests
    effect_voice_runtime_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/effect_voice_runtime.c)
target_include_directories(effect_voice_runtime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(effect_voice_runtime_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(effect_voice_runtime_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(effect_voice_runtime_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(effect_voice_runtime_tests PRIVATE psyz)
add_test(NAME effect_voice_runtime COMMAND effect_voice_runtime_tests)
set_tests_properties(effect_voice_runtime PROPERTIES LABELS unit)

add_executable(forced_voice_playback_tests
    forced_voice_playback_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/forced_voice_playback.c)
target_include_directories(forced_voice_playback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(forced_voice_playback_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(forced_voice_playback_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(forced_voice_playback_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(forced_voice_playback_tests PRIVATE psyz)
add_test(NAME forced_voice_playback COMMAND forced_voice_playback_tests)
set_tests_properties(forced_voice_playback PROPERTIES LABELS unit)

add_executable(sound_cues_tests
    sound_cues_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/audio/sound_cues.c)
target_include_directories(sound_cues_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(sound_cues_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(sound_cues_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(sound_cues_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(sound_cues_tests PRIVATE psyz)
add_test(NAME sound_cues COMMAND sound_cues_tests)
set_tests_properties(sound_cues PROPERTIES LABELS unit)

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
