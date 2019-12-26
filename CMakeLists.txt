cmake_minimum_required(VERSION 3.10)
project(davif)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")

###############################################################################
add_subdirectory(external/libavif-container EXCLUDE_FROM_ALL)
include_directories(external/libavif-container/src)
###############################################################################
add_subdirectory(external/libyuv EXCLUDE_FROM_ALL)
include_directories(external/libyuv/include)
###############################################################################
set(SKIP_INSTALL_ALL ON)
set(ZLIB_TESTS OFF BOOL)
add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
include_directories(external/zlib)
###############################################################################
set(SKIP_INSTALL_ALL ON)
set(PNG_SHARED OFF CACHE BOOL "Build shared lib" FORCE)
set(PNG_STATIC ON CACHE BOOL "Build static lib" FORCE)
set(PNG_TESTS OFF CACHE BOOL "Build libpng tests" FORCE)
add_subdirectory(external/libpng EXCLUDE_FROM_ALL)
#FIXME(ledyba-z): Workaround to include "pnglibconf.h"
target_include_directories(png_static PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/external/libpng)
include_directories(external/libpng)
###############################################################################
include(ExternalProject)
###############################################################################
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    ExternalProject_Add(dav1d_repo
            PREFIX ${CMAKE_BINARY_DIR}
            URL ${CMAKE_SOURCE_DIR}/external/dav1d
            CONFIGURE_COMMAND
            env "CFLAGS=${CMAKE_CXX_FLAGS_DEBUG}" meson setup
            --prefix=${CMAKE_BINARY_DIR}
            --libdir=lib
            --default-library static
            --buildtype=debug
            -Denable_asm=false
            --optimization=g
            "${CMAKE_SOURCE_DIR}/external/dav1d"
            "${CMAKE_BINARY_DIR}/build-dav1d"
            BUILD_COMMAND ninja -C "${CMAKE_BINARY_DIR}/build-dav1d"
            INSTALL_DIR ${CMAKE_BINARY_DIR}
            INSTALL_COMMAND ninja install -C "${CMAKE_BINARY_DIR}/build-dav1d"
            )
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
IF(CMAKE_BUILD_TYPE MATCHES Release)
    ExternalProject_Add(dav1d_repo
            PREFIX ${CMAKE_BINARY_DIR}
            URL ${CMAKE_SOURCE_DIR}/external/dav1d
            CONFIGURE_COMMAND
            meson setup
            --prefix=${CMAKE_BINARY_DIR}
            --libdir=lib
            --default-library static
            --buildtype=release
            -Denable_asm=true
            --optimization=3
            "${CMAKE_SOURCE_DIR}/external/dav1d"
            "${CMAKE_BINARY_DIR}/build-dav1d"
            BUILD_COMMAND ninja -C "${CMAKE_BINARY_DIR}/build-dav1d"
            INSTALL_DIR ${CMAKE_BINARY_DIR}
            INSTALL_COMMAND ninja install -C "${CMAKE_BINARY_DIR}/build-dav1d"
            )
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)
link_directories(${CMAKE_BINARY_DIR}/lib)
include_directories(${CMAKE_BINARY_DIR}/include)
###############################################################################

add_executable(davif
        src/main.cpp
        src/util/StreamWriter.hpp
        src/util/StreamWriter.cpp
        src/util/File.cpp
        src/util/File.h
        )

add_dependencies(davif dav1d_repo)
target_link_libraries(davif libavif-container)
target_link_libraries(davif yuv)
target_link_libraries(davif zlibstatic)
target_link_libraries(davif png_static)
target_link_libraries(davif dav1d)
target_link_libraries(davif pthread)
