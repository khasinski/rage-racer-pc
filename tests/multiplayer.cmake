set(mp_tests protocol prediction connect menu)
if(NOT WIN32)
    list(APPEND mp_tests socket transport)
endif()
foreach(test IN LISTS mp_tests)
    add_executable(mp_${test}_tests ${CMAKE_SOURCE_DIR}/tests/port/mp_${test}_tests.c)
    target_include_directories(mp_${test}_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)
    if(NOT test STREQUAL "transport")
        target_link_libraries(mp_${test}_tests PRIVATE rage-mp-protocol)
    endif()
    if(NOT MSVC)
        target_compile_options(mp_${test}_tests PRIVATE ${RAGE_STRICT_WARNING_OPTIONS})
    endif()
    if(RAGE_ENABLE_SANITIZERS AND
       CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
        target_compile_options(mp_${test}_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(mp_${test}_tests PRIVATE -fsanitize=address,undefined)
    endif()
endforeach()
target_sources(mp_connect_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port/mp_client.c)
target_sources(mp_menu_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/port/mp_menu.c
    ${CMAKE_SOURCE_DIR}/src/main/PAL/main/car/catalog_parse.c)
if(WIN32)
    target_link_libraries(mp_connect_tests PRIVATE ws2_32)
endif()
foreach(test protocol prediction connect menu)
    add_test(NAME mp_${test} COMMAND mp_${test}_tests)
    set_tests_properties(mp_${test} PROPERTIES LABELS "unit;multiplayer" TIMEOUT 10)
endforeach()
if(NOT WIN32)
    add_test(NAME mp_socket COMMAND mp_socket_tests)
    set_tests_properties(mp_socket PROPERTIES
        LABELS "unit;multiplayer" TIMEOUT 5 SKIP_RETURN_CODE 77)
    add_dependencies(mp_transport_tests rage-mp-test-client)
    add_test(NAME mp_transport COMMAND mp_transport_tests $<TARGET_FILE:rage-mp-test-client>)
    set_tests_properties(mp_transport PROPERTIES
        LABELS "integration;multiplayer" TIMEOUT 15 SKIP_RETURN_CODE 77)
endif()
add_test(NAME mp_client_invalid_port COMMAND rage-mp-test-client 127.0.0.1 65536 driver)
add_test(NAME mp_client_invalid_count COMMAND rage-mp-test-client 127.0.0.1 7243 driver -1)
set_tests_properties(mp_client_invalid_port mp_client_invalid_count
    PROPERTIES LABELS "unit;multiplayer" WILL_FAIL TRUE)

# Rust is optional for single-player builds. Enable explicitly on server hosts.
option(RAGE_TEST_SERVER "Register Rust server tests in CTest" OFF)
if(NOT RAGE_TEST_SERVER)
    return()
endif()
if(RAGE_ENABLE_SANITIZERS)
    message(FATAL_ERROR "Server CTest uses non-sanitized C libraries; use a separate build directory")
endif()
find_program(RAGE_CARGO_EXECUTABLE cargo REQUIRED)
get_filename_component(cargo_dir "${RAGE_CARGO_EXECUTABLE}" DIRECTORY)
find_program(RAGE_RUSTC_EXECUTABLE rustc HINTS "${cargo_dir}" REQUIRED)
set(server_test_command ${CMAKE_COMMAND} -E env
    "RUSTC=${RAGE_RUSTC_EXECUTABLE}"
    "RAGE_SIM_LIB_DIR=$<TARGET_FILE_DIR:rage-sim>"
    "CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/server-target"
    "${RAGE_CARGO_EXECUTABLE}" test --release --locked --offline
    --manifest-path "${CMAKE_SOURCE_DIR}/server/Cargo.toml")
add_test(NAME server_unit COMMAND ${server_test_command})
set_tests_properties(server_unit PROPERTIES
    LABELS "unit;multiplayer" RESOURCE_LOCK server_cargo TIMEOUT 180)
if(RAGE_SIM_DISC_BIN)
    add_test(NAME server_retail COMMAND ${CMAKE_COMMAND} -E env
        "RAGE_SIM_DISC_BIN=${RAGE_SIM_DISC_BIN}" ${server_test_command} -- --ignored)
    set_tests_properties(server_retail PROPERTIES
        LABELS "integration;multiplayer" RESOURCE_LOCK server_cargo TIMEOUT 180)
endif()
