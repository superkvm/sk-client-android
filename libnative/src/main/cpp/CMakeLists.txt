
cmake_minimum_required(VERSION 3.10.2)


project("nativelib")

# lame
include_directories(${CMAKE_SOURCE_DIR}/module/mp3/lame)
aux_source_directory(${CMAKE_SOURCE_DIR}/module/mp3/lame SRC_LAME)
# NDK clang is strict C99; LAME often omits libc headers. STDC_HEADERS=1 avoids id3tag.c K&R strchr decls
# conflicting with -include <string.h>.
set_source_files_properties(${SRC_LAME} PROPERTIES
        COMPILE_DEFINITIONS STDC_HEADERS=1
        COMPILE_OPTIONS "-include;stdlib.h;-include;string.h")

add_library(
        nativelib
        SHARED

        ${SRC_LAME}
        utils/logger.cpp
        module/yuv/yuv.cpp
        module/mp3/mp3.cpp
        proxy/proxy_yuv.cpp
        proxy/proxy_mp3.cpp
        nativelib.cpp)

find_library(
        log-lib
        log)

target_link_libraries(
        nativelib
        ${log-lib})