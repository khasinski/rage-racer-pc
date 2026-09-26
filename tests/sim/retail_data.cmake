# Disc images are supplied locally; never bundled with the test suite.
set(RAGE_SIM_DISC_BIN "" CACHE FILEPATH "Legal Track 01 BIN for headless retail data validation")
set(RAGE_SIM_CAR_CATALOG "" CACHE FILEPATH "Optional car catalog for automatic retail race validation")
add_executable(retail_data_tests
    ${CMAKE_SOURCE_DIR}/tests/car/retail_data_tests.c)
target_include_directories(retail_data_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/port)
target_compile_options(retail_data_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(retail_data_tests PRIVATE rage-data)
if(RAGE_SIM_DISC_BIN)
    add_test(NAME retail_data COMMAND retail_data_tests "${RAGE_SIM_DISC_BIN}")
    set_tests_properties(retail_data PROPERTIES LABELS functional)
    add_test(NAME retail_grid COMMAND retail_data_tests "${RAGE_SIM_DISC_BIN}" grid)
    set_tests_properties(retail_grid PROPERTIES LABELS functional)
    if(RAGE_SIM_CAR_CATALOG)
        add_test(NAME retail_catalog COMMAND retail_data_tests
            "${RAGE_SIM_DISC_BIN}" all "${RAGE_SIM_CAR_CATALOG}")
        set_tests_properties(retail_catalog PROPERTIES LABELS functional)
    endif()
endif()

# Retail model import is independent of the race runner and GPU availability.
add_executable(retail_model_tests
    ${CMAKE_SOURCE_DIR}/tests/render/retail_model_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/native_mesh_writer.c
    ${CMAKE_SOURCE_DIR}/src/port/native_car_cache.c
    ${CMAKE_SOURCE_DIR}/src/port/native_texture.c)
target_include_directories(retail_model_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/port)
target_compile_options(retail_model_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(retail_model_tests PRIVATE rage-data rage-rmesh rage-render-world)
if(RAGE_SIM_DISC_BIN)
    add_test(NAME retail_model COMMAND retail_model_tests "${RAGE_SIM_DISC_BIN}")
    set_tests_properties(retail_model PROPERTIES LABELS functional)
endif()

add_executable(client_race_tests
    ${CMAKE_SOURCE_DIR}/tests/render/client_race_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/client_race.c
    ${CMAKE_SOURCE_DIR}/src/port/client_world.c
    ${CMAKE_SOURCE_DIR}/src/port/native_visibility.c
    ${CMAKE_SOURCE_DIR}/src/port/client_assets.c
    ${CMAKE_SOURCE_DIR}/src/port/client_frame.c
    ${CMAKE_SOURCE_DIR}/src/port/modern/client_frame_gpu.c
    ${CMAKE_SOURCE_DIR}/tests/render/client_frame_source_tests.c
    ${CMAKE_SOURCE_DIR}/src/port/native_sky.c
    ${CMAKE_SOURCE_DIR}/src/port/sky_panorama_layout.c
    ${CMAKE_SOURCE_DIR}/src/port/modern/modern_prepared_meshes.c
    ${CMAKE_SOURCE_DIR}/src/port/native_texture.c
    ${CMAKE_SOURCE_DIR}/src/port/native_mesh_writer.c
    ${CMAKE_SOURCE_DIR}/src/port/native_car_cache.c
    ${CMAKE_SOURCE_DIR}/src/port/race_view.c
    ${CMAKE_SOURCE_DIR}/src/port/car_parts.c)
target_include_directories(client_race_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/render
    ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/port ${CMAKE_SOURCE_DIR}/src/port/include)
target_compile_options(client_race_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
target_link_libraries(client_race_tests PRIVATE rage-data rage-rmesh rage-render-world)
if(RAGE_SIM_DISC_BIN)
    add_test(NAME client_race COMMAND client_race_tests "${RAGE_SIM_DISC_BIN}")
    set_tests_properties(client_race PROPERTIES LABELS functional)
    add_test(NAME retail_materials COMMAND client_race_tests "${RAGE_SIM_DISC_BIN}" materials)
    set_tests_properties(retail_materials PROPERTIES LABELS functional)
endif()
