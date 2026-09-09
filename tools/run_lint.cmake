# Run host-only static analysis from CMake's compilation database.
if(NOT DEFINED BUILD_DIR OR NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "BUILD_DIR and SOURCE_DIR are required")
endif()

set(database "${BUILD_DIR}/compile_commands.json")
if(NOT EXISTS "${database}")
    message(FATAL_ERROR "missing compilation database: ${database}")
endif()
find_program(CLANG_TIDY_EXECUTABLE clang-tidy REQUIRED)
find_program(CPPCHECK_EXECUTABLE cppcheck REQUIRED)

file(READ "${database}" commands)
string(JSON command_count LENGTH "${commands}")
set(host_sources)
foreach(index RANGE 0 ${command_count})
    if(index EQUAL command_count)
        break()
    endif()
    string(JSON source GET "${commands}" ${index} file)
    file(REAL_PATH "${source}" source)
    file(RELATIVE_PATH relative "${SOURCE_DIR}" "${source}")
    if(relative MATCHES "^src/(port|render)/.*[.]c$")
        list(APPEND host_sources "${source}")
    endif()
endforeach()
if(NOT host_sources)
    message(FATAL_ERROR "compilation database contains no host lint sources")
endif()
list(REMOVE_DUPLICATES host_sources)

execute_process(COMMAND "${CLANG_TIDY_EXECUTABLE}" -p "${BUILD_DIR}" --quiet ${host_sources}
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "clang-tidy failed (${result})")
endif()
execute_process(COMMAND "${CPPCHECK_EXECUTABLE}"
    --enable=warning,style,performance,portability --error-exitcode=1
    --inline-suppr --suppress=missingIncludeSystem --std=c11
    -DRAGE_HOST_PORT=1 -D__psyz=1
    "-I${SOURCE_DIR}/src" "-I${SOURCE_DIR}/src/port"
    "-I${SOURCE_DIR}/src/port/include" "-I${SOURCE_DIR}/include"
    ${host_sources} RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "cppcheck failed (${result})")
endif()
