# Tests are grouped by what they are about, not by how they run. A unit test in
# C and the end-to-end script covering the same subsystem live in the same
# directory; ctest -L functional/e2e/tooling still answers how a test runs.
#
# Shared fixtures live in support/: common.h and the include/ tree the C tests
# compile against, and the Python helpers the end-to-end scripts import.

# Test timeouts are wall-clock, and most of these run the game at its own pace
# rather than as fast as the machine can. race_cdda waits out an audio track and
# needs 66 seconds of real time; its limit was 45, so it failed whenever anything
# else was running. Every limit here is three times the time its test takes on an
# idle machine, which is enough that a busy one does not turn a passing test into
# a failing one. They still catch a hang, which is what they are for.
#
# The scripts carry their own subprocess timeouts as well, and those follow the
# same rule. They were left at one times the idle cost when these were raised,
# which made them the limit that actually bit: scenario_after_finish had 65
# seconds inside the script against 210 here, so a parallel run failed it while
# ctest was still waiting patiently.

# Visual-comparison helpers remain optional developer tools. Do not register
# them as failing tests on hosts that intentionally ship only the C/Python
# toolchain used by the port and release jobs.
find_program(RAGE_RUBY_EXECUTABLE ruby)

# A release build defines NDEBUG, which would turn the assertion-based C
# tests into no-ops. Keep assertions active in every test target while leaving
# the release game binaries untouched.
if(MSVC)
    add_compile_options(/UNDEBUG)
else()
    add_compile_options(-UNDEBUG)
endif()

# Several test targets link the object libraries the top level compiles with
# sanitizers, and an object built with them needs the runtime on its link
# line. The top level adds that only to the two game binaries, so the tests
# ask for it here rather than each target remembering.
if(RAGE_ENABLE_SANITIZERS AND
   CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(audio)
add_subdirectory(car)
add_subdirectory(disc)
add_subdirectory(input)
add_subdirectory(menu)
add_subdirectory(port)
add_subdirectory(race)
add_subdirectory(render)
add_subdirectory(tooling)
