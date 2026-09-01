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
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/save/save_header.c)
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
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/prize_money_screen.c)
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
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/menu/logo_screens.c)
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
        -Wall -Wextra -Wno-error)
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
