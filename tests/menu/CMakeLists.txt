add_executable(team_logo_tests
    team_logo_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/team_logo.c)
target_include_directories(team_logo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(team_logo_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(team_logo_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(team_logo_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(team_logo_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
add_test(NAME team_logo COMMAND team_logo_tests)

set_tests_properties(team_logo PROPERTIES LABELS functional)

add_executable(time_attack_plate_tests
    time_attack_plate_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/time_attack_plate.c)
target_include_directories(time_attack_plate_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include)
target_compile_definitions(time_attack_plate_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(time_attack_plate_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(time_attack_plate_tests PRIVATE psyz)
add_test(NAME time_attack_plate COMMAND time_attack_plate_tests)
set_tests_properties(time_attack_plate PROPERTIES LABELS unit)

add_executable(save_header_tests
    save_header_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_header.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_checksum.c)
target_include_directories(save_header_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(save_header_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(save_header_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(save_header_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(save_header_tests PRIVATE psyz)
add_test(NAME save_header COMMAND save_header_tests)
set_tests_properties(save_header PROPERTIES LABELS unit)

add_executable(save_icon_tests
    save_icon_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_icon.c)
target_include_directories(save_icon_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/support/save_icon
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(save_icon_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(save_icon_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME save_icon COMMAND save_icon_tests)
set_tests_properties(save_icon PROPERTIES LABELS unit)

add_executable(memory_card_io_tests
    memory_card_io_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_io.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_status.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_checksum.c)
target_include_directories(memory_card_io_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(memory_card_io_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(memory_card_io_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(memory_card_io_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME memory_card_io COMMAND memory_card_io_tests)
set_tests_properties(memory_card_io PROPERTIES LABELS unit)

add_executable(memory_card_runtime_tests
    memory_card_runtime_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_runtime.c)
target_include_directories(memory_card_runtime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(memory_card_runtime_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(memory_card_runtime_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(memory_card_runtime_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME memory_card_runtime COMMAND memory_card_runtime_tests)
set_tests_properties(memory_card_runtime PROPERTIES LABELS unit)

add_executable(memory_card_event_polling_tests
    memory_card_event_polling_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_event_polling.c)
target_include_directories(memory_card_event_polling_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(memory_card_event_polling_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(memory_card_event_polling_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(memory_card_event_polling_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME memory_card_event_polling COMMAND memory_card_event_polling_tests)
set_tests_properties(memory_card_event_polling PROPERTIES LABELS unit)

add_test(NAME car_prices COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/verify_car_prices.py ${CMAKE_SOURCE_DIR})

set_tests_properties(car_prices PROPERTIES LABELS functional)

if(RAGE_RUBY_EXECUTABLE)
add_test(
    NAME native_ui_script_layout
    COMMAND ruby
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_native_ui_script_layout.rb
        ${CMAKE_SOURCE_DIR}
)

set_tests_properties(native_ui_script_layout PROPERTIES LABELS functional)
endif()

if(TARGET rage-host-state)
# The prize screen's arithmetic, which is what a race leaves behind.
add_executable(prize_screen_tests
    prize_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/prize_money_screen.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/race/prize_money.c)
target_include_directories(prize_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(prize_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(prize_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(prize_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(prize_screen_tests PRIVATE -Wall -Wextra -Wno-error)
endif()
target_link_libraries(prize_screen_tests PRIVATE psyz)
add_test(NAME prize_screen COMMAND prize_screen_tests)
set_tests_properties(prize_screen PROPERTIES LABELS functional)

# The memory card menu, swept across its whole state space.
add_executable(memory_card_menu_tests
    memory_card_menu_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_format_actions.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_menu_frame.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_slot_actions.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_state_actions.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/update_memory_card_menu.c)
target_include_directories(memory_card_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(memory_card_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(memory_card_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(memory_card_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(memory_card_menu_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(memory_card_menu_tests PRIVATE psyz)
add_test(NAME memory_card_menu COMMAND memory_card_menu_tests)
set_tests_properties(memory_card_menu PROPERTIES LABELS functional)

add_executable(memory_card_screen_tests
    memory_card_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/draw_memory_card_screen.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/draw_memory_card_message.c)
target_include_directories(memory_card_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(memory_card_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(memory_card_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(memory_card_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME memory_card_screen COMMAND memory_card_screen_tests)
set_tests_properties(memory_card_screen PROPERTIES LABELS unit)

# The logo editor's two halves, swept across the buttons that act on them.
add_executable(team_logo_editor_tests
    team_logo_editor_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/team_logo_editor.c)
target_include_directories(team_logo_editor_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(team_logo_editor_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(team_logo_editor_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(team_logo_editor_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(team_logo_editor_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(team_logo_editor_tests PRIVATE psyz)
add_test(NAME team_logo_editor COMMAND team_logo_editor_tests)
set_tests_properties(team_logo_editor PROPERTIES LABELS functional)

add_executable(team_logo_transform_tests
    team_logo_transform_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/team_logo_transform.c)
target_include_directories(team_logo_transform_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(team_logo_transform_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(team_logo_transform_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(team_logo_transform_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(team_logo_transform_tests PRIVATE psyz)
add_test(NAME team_logo_transform COMMAND team_logo_transform_tests)
set_tests_properties(team_logo_transform PROPERTIES LABELS functional)

add_executable(draw_team_logo_canvas_tests
    draw_team_logo_canvas_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_team_logo_canvas.c)
target_include_directories(draw_team_logo_canvas_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(draw_team_logo_canvas_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(draw_team_logo_canvas_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(draw_team_logo_canvas_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(draw_team_logo_canvas_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(draw_team_logo_canvas_tests PRIVATE psyz)
add_test(NAME draw_team_logo_canvas COMMAND draw_team_logo_canvas_tests)
set_tests_properties(draw_team_logo_canvas PROPERTIES LABELS functional)

add_executable(team_logo_screen_tests
    team_logo_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/team_logo_screen.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/logo_sample_screen.c)
target_include_directories(team_logo_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(team_logo_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(team_logo_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(team_logo_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(team_logo_screen_tests PRIVATE psyz)
add_test(NAME team_logo_screen COMMAND team_logo_screen_tests)
set_tests_properties(team_logo_screen PROPERTIES LABELS functional)

add_executable(team_name_screen_tests
    team_name_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/team_name_screen.c)
target_include_directories(team_name_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(team_name_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(team_name_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(team_name_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(team_name_screen_tests PRIVATE psyz)
add_test(NAME team_name_screen COMMAND team_name_screen_tests)
set_tests_properties(team_name_screen PROPERTIES LABELS functional)

add_executable(paint_color_screen_tests
    paint_color_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/paint_color_screen.c)
target_include_directories(paint_color_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(paint_color_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(paint_color_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(paint_color_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(paint_color_screen_tests PRIVATE psyz)
add_test(NAME paint_color_screen COMMAND paint_color_screen_tests)
set_tests_properties(paint_color_screen PROPERTIES LABELS functional)

add_executable(car_shop_menu_tests
    car_shop_menu_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/car_shop_menu.c)
target_include_directories(car_shop_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_shop_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_shop_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_shop_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_shop_menu_tests PRIVATE psyz)
add_test(NAME car_shop_menu COMMAND car_shop_menu_tests)
set_tests_properties(car_shop_menu PROPERTIES LABELS functional)

add_executable(engineer_shop_menu_tests
    engineer_shop_menu_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/engineer_shop_menu.c)
target_include_directories(engineer_shop_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(engineer_shop_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(engineer_shop_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(engineer_shop_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(engineer_shop_menu_tests PRIVATE psyz)
add_test(NAME engineer_shop_menu COMMAND engineer_shop_menu_tests)
set_tests_properties(engineer_shop_menu PROPERTIES LABELS functional)

add_executable(shop_price_panel_tests
    shop_price_panel_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/shop_price_panel.c)
target_include_directories(shop_price_panel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(shop_price_panel_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(shop_price_panel_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(shop_price_panel_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(shop_price_panel_tests PRIVATE psyz)
add_test(NAME shop_price_panel COMMAND shop_price_panel_tests)
set_tests_properties(shop_price_panel PROPERTIES LABELS unit)

add_executable(car_engine_spec_tests
    car_engine_spec_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_car_engine_spec.c)
target_include_directories(car_engine_spec_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_engine_spec_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_engine_spec_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_engine_spec_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(car_engine_spec_tests PRIVATE psyz)
add_test(NAME car_engine_spec COMMAND car_engine_spec_tests)
set_tests_properties(car_engine_spec PROPERTIES LABELS unit)

add_executable(customize_menu_tests
    customize_menu_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/customize_menu.c)
target_include_directories(customize_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(customize_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(customize_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(customize_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(customize_menu_tests PRIVATE psyz)
add_test(NAME customize_menu COMMAND customize_menu_tests)
set_tests_properties(customize_menu PROPERTIES LABELS functional)

add_executable(car_select_menu_tests
    car_select_menu_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/car_select_menu.c)
target_include_directories(car_select_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_select_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_select_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_select_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(car_select_menu_tests PRIVATE psyz)
add_test(NAME car_select_menu COMMAND car_select_menu_tests)
set_tests_properties(car_select_menu PROPERTIES LABELS unit)

add_executable(ranking_screen_tests
    ranking_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/ranking_screen.c)
target_include_directories(ranking_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(ranking_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(ranking_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(ranking_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(ranking_screen_tests PRIVATE psyz)
add_test(NAME ranking_screen COMMAND ranking_screen_tests)
set_tests_properties(ranking_screen PROPERTIES LABELS functional)

add_executable(car_select_state_tests
    car_select_state_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/car_select.c)
target_include_directories(car_select_state_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_select_state_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_select_state_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_select_state_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(car_select_state_tests PRIVATE psyz)
add_test(NAME car_select_state COMMAND car_select_state_tests)
set_tests_properties(car_select_state PROPERTIES LABELS unit)

add_executable(tire_compound_slider_tests
    tire_compound_slider_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/tire_compound_slider.c)
target_include_directories(tire_compound_slider_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(tire_compound_slider_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(tire_compound_slider_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(tire_compound_slider_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(tire_compound_slider_tests PRIVATE psyz)
add_test(NAME tire_compound_slider COMMAND tire_compound_slider_tests)
set_tests_properties(tire_compound_slider PROPERTIES LABELS unit)

add_executable(browse_arrows_tests
    browse_arrows_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/browse_arrows.c)
target_include_directories(browse_arrows_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(browse_arrows_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(browse_arrows_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(browse_arrows_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(browse_arrows_tests PRIVATE psyz)
add_test(NAME browse_arrows COMMAND browse_arrows_tests)
set_tests_properties(browse_arrows PROPERTIES LABELS unit)

add_executable(car_spec_graph_tests
    car_spec_graph_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/car_spec_graph.c)
target_include_directories(car_spec_graph_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_spec_graph_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_spec_graph_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_spec_graph_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(car_spec_graph_tests PRIVATE psyz)
add_test(NAME car_spec_graph COMMAND car_spec_graph_tests)
set_tests_properties(car_spec_graph PROPERTIES LABELS unit)

add_executable(car_select_screen_tests
    car_select_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/car_select_screen.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/menu_turntable.c)
target_include_directories(car_select_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_select_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_select_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_select_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_select_screen_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_select_screen_tests PRIVATE psyz)
add_test(NAME car_select_screen COMMAND car_select_screen_tests)
set_tests_properties(car_select_screen PROPERTIES LABELS functional)

# The clock every menu animation runs on, before it is taken apart.
add_executable(timed_draw_script_tests
    timed_draw_script_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/render/scripted_draw.c)
target_include_directories(timed_draw_script_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_definitions(timed_draw_script_tests PRIVATE __psyz=1)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(timed_draw_script_tests PRIVATE
        "-iquote${CMAKE_SOURCE_DIR}/src/port/include"
        "-iquote${CMAKE_SOURCE_DIR}/include")
endif()
target_link_libraries(timed_draw_script_tests PRIVATE psyz)
add_test(NAME timed_draw_script COMMAND timed_draw_script_tests)
set_tests_properties(timed_draw_script PROPERTIES LABELS functional)

add_executable(menu_button_tests
    menu_button_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/menu_button.c)
target_include_directories(menu_button_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(menu_button_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(menu_button_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(menu_button_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(menu_button_tests PRIVATE psyz)
add_test(NAME menu_button COMMAND menu_button_tests)
set_tests_properties(menu_button PROPERTIES LABELS functional)

add_executable(class_change_curtain_tests
    class_change_curtain_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_class_change_curtain.c)
target_include_directories(class_change_curtain_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(class_change_curtain_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(class_change_curtain_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(class_change_curtain_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(class_change_curtain_tests PRIVATE psyz)
add_test(NAME class_change_curtain COMMAND class_change_curtain_tests)
set_tests_properties(class_change_curtain PROPERTIES LABELS functional)

add_executable(menu_alt_panel_tests
    menu_alt_panel_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_menu_alt_panel.c)
target_include_directories(menu_alt_panel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(menu_alt_panel_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(menu_alt_panel_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(menu_alt_panel_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(menu_alt_panel_tests PRIVATE psyz)
add_test(NAME menu_alt_panel COMMAND menu_alt_panel_tests)
set_tests_properties(menu_alt_panel PROPERTIES LABELS functional)

add_executable(menu_hints_tests
    menu_hints_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/menu_hints.c)
target_include_directories(menu_hints_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(menu_hints_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(menu_hints_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(menu_hints_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(menu_hints_tests PRIVATE psyz)
add_test(NAME menu_hints COMMAND menu_hints_tests)
set_tests_properties(menu_hints PROPERTIES LABELS functional)

add_executable(count_owned_cars_tests
    count_owned_cars_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/count_owned_cars.c)
target_include_directories(count_owned_cars_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(count_owned_cars_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(count_owned_cars_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(count_owned_cars_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME count_owned_cars COMMAND count_owned_cars_tests)
set_tests_properties(count_owned_cars PROPERTIES LABELS unit)

add_executable(ending_still_tests
    ending_still_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/ending_still.c)
target_include_directories(ending_still_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(ending_still_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(ending_still_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(ending_still_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME ending_still COMMAND ending_still_tests)
set_tests_properties(ending_still PROPERTIES LABELS functional)

add_executable(course_select_screen_tests
    course_select_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/course_select_screen.c)
target_include_directories(course_select_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(course_select_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(course_select_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(course_select_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(course_select_screen_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(course_select_screen_tests PRIVATE psyz)
add_test(NAME course_select_screen COMMAND course_select_screen_tests)
set_tests_properties(course_select_screen PROPERTIES LABELS functional)

add_executable(course_navigation_tests
    course_navigation_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/course_navigation.c)
target_include_directories(course_navigation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(course_navigation_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(course_navigation_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(course_navigation_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME course_navigation COMMAND course_navigation_tests)
set_tests_properties(course_navigation PROPERTIES LABELS unit)

add_executable(car_shop_screen_tests
    car_shop_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/car_shop_screen.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/shop_prompt.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/menu_turntable.c)
target_include_directories(car_shop_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(car_shop_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(car_shop_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(car_shop_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(car_shop_screen_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(car_shop_screen_tests PRIVATE psyz)
add_test(NAME car_shop_screen COMMAND car_shop_screen_tests)
set_tests_properties(car_shop_screen PROPERTIES LABELS functional)

add_executable(engineer_shop_screen_tests
    engineer_shop_screen_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/engineer_shop_screen.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/shop_prompt.c)
target_include_directories(engineer_shop_screen_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(engineer_shop_screen_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(engineer_shop_screen_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(engineer_shop_screen_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
    target_compile_options(engineer_shop_screen_tests PRIVATE
        -Wall -Wextra -Wno-error)
endif()
target_link_libraries(engineer_shop_screen_tests PRIVATE psyz)
add_test(NAME engineer_shop_screen COMMAND engineer_shop_screen_tests)
set_tests_properties(engineer_shop_screen PROPERTIES LABELS functional)

add_executable(logo_name_panel_tests
    logo_name_panel_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_logo_sample_panel.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_team_name_entry.c)
target_include_directories(logo_name_panel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(logo_name_panel_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(logo_name_panel_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(logo_name_panel_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(logo_name_panel_tests PRIVATE psyz)
add_test(NAME logo_name_panel COMMAND logo_name_panel_tests)
set_tests_properties(logo_name_panel PROPERTIES LABELS unit)

add_executable(paint_palette_counter_tests
    paint_palette_counter_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_paint_color_palette.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/draw_owned_car_counter.c)
target_include_directories(paint_palette_counter_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(paint_palette_counter_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(paint_palette_counter_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(paint_palette_counter_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(paint_palette_counter_tests PRIVATE psyz)
add_test(NAME paint_palette_counter COMMAND paint_palette_counter_tests)
set_tests_properties(paint_palette_counter PROPERTIES LABELS unit)

add_executable(menu_light_burst_tests
    menu_light_burst_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/menu_light_burst.c)
target_include_directories(menu_light_burst_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(menu_light_burst_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(menu_light_burst_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(menu_light_burst_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(menu_light_burst_tests PRIVATE psyz)
add_test(NAME menu_light_burst COMMAND menu_light_burst_tests)
set_tests_properties(menu_light_burst PROPERTIES LABELS unit)

add_executable(ranking_table_tests
    ranking_table_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/ranking_table.c)
target_include_directories(ranking_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(ranking_table_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(ranking_table_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(ranking_table_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(ranking_table_tests PRIVATE psyz)
add_test(NAME ranking_table COMMAND ranking_table_tests)
set_tests_properties(ranking_table PROPERTIES LABELS unit)

add_executable(frontend_tests
    frontend_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/frontend.c)
target_include_directories(frontend_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(frontend_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(frontend_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(frontend_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(frontend_tests PRIVATE psyz)
add_test(NAME frontend COMMAND frontend_tests)
set_tests_properties(frontend PROPERTIES LABELS functional)

add_executable(volume_bar_tests
    volume_bar_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/volume_bar.c)
target_include_directories(volume_bar_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(volume_bar_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(volume_bar_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(volume_bar_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(volume_bar_tests PRIVATE psyz)
add_test(NAME volume_bar COMMAND volume_bar_tests)
set_tests_properties(volume_bar PROPERTIES LABELS unit)

add_executable(sound_option_menu_tests
    sound_option_menu_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/sound_option_menu.c)
target_include_directories(sound_option_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(sound_option_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(sound_option_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(sound_option_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(sound_option_menu_tests PRIVATE psyz)
add_test(NAME sound_option_menu COMMAND sound_option_menu_tests)
set_tests_properties(sound_option_menu PROPERTIES LABELS functional)

add_executable(screen_adjust_tests
    screen_adjust_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/screen_adjust.c)
target_include_directories(screen_adjust_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(screen_adjust_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(screen_adjust_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(screen_adjust_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(screen_adjust_tests PRIVATE psyz)
add_test(NAME screen_adjust COMMAND screen_adjust_tests)
set_tests_properties(screen_adjust PROPERTIES LABELS functional)

add_executable(option_root_menu_tests
    option_root_menu_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/option_root_menu.c)
target_include_directories(option_root_menu_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(option_root_menu_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(option_root_menu_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(option_root_menu_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(option_root_menu_tests PRIVATE psyz)
add_test(NAME option_root_menu COMMAND option_root_menu_tests)
set_tests_properties(option_root_menu PROPERTIES LABELS functional)

add_executable(course_card_tests
    course_card_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/course_card.c)
target_include_directories(course_card_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(course_card_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(course_card_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(course_card_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
target_link_libraries(course_card_tests PRIVATE psyz)
add_test(NAME course_card COMMAND course_card_tests)
set_tests_properties(course_card PROPERTIES LABELS unit)

add_executable(negcon_calibration_update_tests
    negcon_calibration_update_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/negcon_calibration_update.c)
target_include_directories(negcon_calibration_update_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(negcon_calibration_update_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(negcon_calibration_update_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(negcon_calibration_update_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME negcon_calibration_update COMMAND negcon_calibration_update_tests)
set_tests_properties(negcon_calibration_update PROPERTIES LABELS unit)

add_executable(controller_config_update_tests
    controller_config_update_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/controller_config_update.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/negcon_calibration_state.c)
target_include_directories(controller_config_update_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(controller_config_update_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(controller_config_update_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(controller_config_update_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME controller_config_update COMMAND controller_config_update_tests)
set_tests_properties(controller_config_update PROPERTIES LABELS unit)

add_executable(controller_config_draw_tests
    controller_config_draw_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/controller_config_draw.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/pad/pad_config_screen.c)
target_include_directories(controller_config_draw_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(controller_config_draw_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(controller_config_draw_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(controller_config_draw_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME controller_config_draw COMMAND controller_config_draw_tests)
set_tests_properties(controller_config_draw PROPERTIES LABELS unit)

add_executable(controller_config_widgets_tests
    controller_config_widgets_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/pad/controller_config_widgets.c)
target_include_directories(controller_config_widgets_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(controller_config_widgets_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(controller_config_widgets_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(controller_config_widgets_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME controller_config_widgets COMMAND controller_config_widgets_tests)
set_tests_properties(controller_config_widgets PROPERTIES LABELS unit)

add_executable(controller_setup_scene_tests
    controller_setup_scene_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/pad/controller_setup_scene.c)
target_include_directories(controller_setup_scene_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(controller_setup_scene_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(controller_setup_scene_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(controller_setup_scene_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME controller_setup_scene COMMAND controller_setup_scene_tests)
set_tests_properties(controller_setup_scene PROPERTIES LABELS unit)

add_executable(memory_card_menu_helpers_tests
    memory_card_menu_helpers_tests.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/draw_memory_card_save_rows.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_menu_controls.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/memory_card_menu_lifecycle.c)
target_include_directories(memory_card_menu_helpers_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/port/include
    ${CMAKE_SOURCE_DIR}/external/psyz/psyz/include
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(memory_card_menu_helpers_tests PRIVATE __psyz=1)
if(MSVC)
    target_compile_options(memory_card_menu_helpers_tests PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h")
else()
    target_compile_options(memory_card_menu_helpers_tests PRIVATE
        "-include${CMAKE_SOURCE_DIR}/src/port/include/rage/compat.h"
        -Wall -Wextra -Werror)
endif()
add_test(NAME memory_card_menu_helpers COMMAND memory_card_menu_helpers_tests)
set_tests_properties(memory_card_menu_helpers PROPERTIES LABELS unit)
endif()

if(TARGET rage-racer-smoke)
    add_test(
        NAME menu_sweep
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_menu_sweep.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(menu_sweep PROPERTIES LABELS e2e)
    set_tests_properties(menu_sweep PROPERTIES TIMEOUT 225)

    add_test(
        NAME trophy_view
        COMMAND python3
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_trophy_view.py
            $<TARGET_FILE:rage-racer-smoke>
            ${CMAKE_SOURCE_DIR}
    )
set_tests_properties(trophy_view PROPERTIES LABELS e2e)
    set_tests_properties(trophy_view PROPERTIES TIMEOUT 165)
endif()
