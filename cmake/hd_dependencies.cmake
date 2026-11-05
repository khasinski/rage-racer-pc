include(FetchContent)
# Ship the decoder statically; players do not need an image-codec installation.
set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_CWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_DWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_GIF2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_IMG2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_VWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPINFO OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rage_webp
    URL https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.6.0.tar.gz
    URL_HASH SHA256=e4ab7009bf0629fd11982d4c2aa83964cf244cffba7347ecd39019a9e38c4564)
FetchContent_MakeAvailable(rage_webp)
add_library(rage-hd-image STATIC ${PROJECT_SOURCE_DIR}/src/port/modern/modern_image.c
    ${PROJECT_SOURCE_DIR}/src/port/modern/modern_png.c)
target_include_directories(rage-hd-image PUBLIC ${PROJECT_SOURCE_DIR}/src PRIVATE ${rage_webp_SOURCE_DIR}/src)
target_link_libraries(rage-hd-image PUBLIC SDL3::SDL3 PRIVATE webpdecoder)

add_library(rage-hd-network INTERFACE)
if(ANDROID)
    target_compile_definitions(rage-hd-network INTERFACE RAGE_HD_NETWORK_DISABLED=1)
elseif(APPLE)
    target_link_libraries(rage-hd-network INTERFACE "-lcurl")
elseif(WIN32)
    # The operating system's WinHTTP stack provides TLS, certificate
    # validation and proxy support. Do not statically link a curl/Schannel
    # client into the game: that made Microsoft Defender's ML heuristics
    # classify Rage-Racer.exe as a downloader trojan.
    target_link_libraries(rage-hd-network INTERFACE winhttp)
else()
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(CURL_USE_SCHANNEL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
    set(CURL_USE_GSSAPI OFF CACHE BOOL "" FORCE)
    set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)
    FetchContent_Declare(rage_curl
        URL https://github.com/curl/curl/releases/download/curl-8_17_0/curl-8.17.0.tar.gz
        URL_HASH SHA256=e8e74cdeefe5fb78b3ae6e90cd542babf788fa9480029cfcee6fd9ced42b7910)
    FetchContent_MakeAvailable(rage_curl)
    target_link_libraries(rage-hd-network INTERFACE CURL::libcurl)
endif()

if(NOT ANDROID)
    add_executable(rage-hd-optimize ${PROJECT_SOURCE_DIR}/tools/hd_pack_optimize.c
        ${PROJECT_SOURCE_DIR}/src/port/hd_pack.c)
    target_include_directories(rage-hd-optimize PRIVATE ${PROJECT_SOURCE_DIR}/src ${rage_webp_SOURCE_DIR}/src)
    target_link_libraries(rage-hd-optimize PRIVATE rage-render-world rage-content rage-hd-image rage-json webp SDL3::SDL3)
endif()
if(NOT ANDROID)
    add_executable(rage-hd-publish ${PROJECT_SOURCE_DIR}/tools/hd_pack_publish.c
        ${PROJECT_SOURCE_DIR}/src/port/hd_pack.c)
    target_include_directories(rage-hd-publish PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(rage-hd-publish PRIVATE rage-render-world rage-content rage-hd-image rage-json SDL3::SDL3)
    # Development/test origin only; it is never packaged with the game.
    add_executable(rage-hd-server ${PROJECT_SOURCE_DIR}/server/hd_server.c)
    target_link_libraries(rage-hd-server PRIVATE SDL3::SDL3)
    if(WIN32)
        target_link_libraries(rage-hd-server PRIVATE ws2_32)
        target_compile_definitions(rage-hd-server PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()
if(NOT ANDROID)
    add_executable(rage-hd-fetch ${PROJECT_SOURCE_DIR}/tools/hd_pack_fetch.c
        ${PROJECT_SOURCE_DIR}/src/port/hd_download.c ${PROJECT_SOURCE_DIR}/src/port/hd_pack.c)
    target_include_directories(rage-hd-fetch PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(rage-hd-fetch PRIVATE rage-render-world rage-content rage-json rage-hd-image rage-hd-network SDL3::SDL3)
endif()
