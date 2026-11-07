# Only the native online client needs WebRTC. All dependencies ship statically;
# none requires an installer, browser, interpreter or system TLS development kit.
include(FetchContent)
set(CMAKE_DISABLE_FIND_PACKAGE_Python3 TRUE)
set(GEN_FILES OFF CACHE BOOL "Use committed Mbed TLS generated C files" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "Mbed TLS tests" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "Mbed TLS programs" FORCE)
set(DISABLE_PACKAGE_CONFIG_AND_INSTALL OFF CACHE BOOL "Export static Mbed TLS targets" FORCE)
set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
FetchContent_Declare(rage_mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG e185d7fd85499c8ce5ca2a54f5cf8fe7dbe3f8df
    GIT_SUBMODULES framework)
FetchContent_MakeAvailable(rage_mbedtls)
target_compile_definitions(mbedtls PUBLIC MBEDTLS_SSL_DTLS_SRTP)
add_library(MbedTLS::MbedTLS ALIAS mbedtls)
set(USE_MBEDTLS ON CACHE BOOL "" FORCE)
set(NO_MEDIA ON CACHE BOOL "" FORCE)
set(NO_WEBSOCKET ON CACHE BOOL "" FORCE)
set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(NO_TESTS ON CACHE BOOL "" FORCE)
FetchContent_Declare(rage_datachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG 6b1e2e620f1e37f0eafeee702eaea0043cb305fd
    GIT_SUBMODULES deps/plog deps/usrsctp deps/libjuice)
FetchContent_MakeAvailable(rage_datachannel)
