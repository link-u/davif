cmake_minimum_required(VERSION 3.13)
project(davif)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")

###############################################################################
add_subdirectory(external/libavif-container EXCLUDE_FROM_ALL)
###############################################################################
# zlib
set(ASM686L OFF CACHE BOOL "Enable building i686 assembly implementation" FORCE)
set(ASM686 OFF CACHE BOOL "Enable building amd64 assembly implementation" FORCE)
add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib" "${CMAKE_CURRENT_BINARY_DIR}/external/zlib")
set(ZLIB_ROOT "${CMAKE_BINARY_DIR}/zlib")
set(ZLIB_DIR "${CMAKE_BINARY_DIR}/zlib")
###############################################################################
set(SKIP_INSTALL_ALL ON)

set(PNG_BUILD_ZLIB ON CACHE BOOL "Custom zlib Location, else find_package is used" FORCE)
set(PNG_SHARED OFF CACHE BOOL "Build shared lib" FORCE)
set(PNG_STATIC ON CACHE BOOL "Build static lib" FORCE)
set(PNG_TESTS OFF CACHE BOOL "Build libpng tests" FORCE)
add_subdirectory(external/libpng EXCLUDE_FROM_ALL)
#FIXME(ledyba-z): Workaround to include "pnglibconf.h"
target_include_directories(png_static PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/external/libpng")
# zlib
target_link_directories(png_static PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/external/zlib")
target_include_directories(png_static PRIVATE external/zlib "${CMAKE_CURRENT_BINARY_DIR}/external/zlib")
target_link_libraries(png_static zlibstatic)
add_dependencies(png_static zlibstatic)
###############################################################################
include(ExternalProject)
###############################################################################
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
	    #-optimization=3
            "${CMAKE_SOURCE_DIR}/external/dav1d"
            "${CMAKE_BINARY_DIR}/build-dav1d"
            BUILD_COMMAND ninja -C "${CMAKE_BINARY_DIR}/build-dav1d"
            INSTALL_DIR ${CMAKE_BINARY_DIR}
            INSTALL_COMMAND ninja install -C "${CMAKE_BINARY_DIR}/build-dav1d"
            )
ELSE()
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
	    #--optimization=g
            "${CMAKE_SOURCE_DIR}/external/dav1d"
            "${CMAKE_BINARY_DIR}/build-dav1d"
            BUILD_COMMAND ninja -C "${CMAKE_BINARY_DIR}/build-dav1d"
            INSTALL_DIR ${CMAKE_BINARY_DIR}
            INSTALL_COMMAND ninja install -C "${CMAKE_BINARY_DIR}/build-dav1d"
            )
ENDIF()
###############################################################################

add_executable(davif
        src/img/Conversion.cpp
        src/img/Conversion.hpp

        src/img/PNGWriter.cpp
        src/img/PNGWriter.hpp

        src/main.cpp
)

add_dependencies(davif dav1d_repo)
add_dependencies(davif lcms_repo)

target_include_directories(davif PRIVATE external/libpng)
target_include_directories(davif PRIVATE "${CMAKE_BINARY_DIR}/include")
target_include_directories(davif PRIVATE external/libavif-container/src)
target_include_directories(davif PRIVATE external/libyuv/include)

target_link_directories(davif PRIVATE "${CMAKE_BINARY_DIR}/lib")
target_link_libraries(davif PRIVATE lcms2)
target_link_libraries(davif PRIVATE libavif-container)
target_link_libraries(davif PRIVATE fmt::fmt)
target_link_libraries(davif PRIVATE zlibstatic)
target_link_libraries(davif PRIVATE png_static)
target_link_libraries(davif PRIVATE dav1d)
target_link_libraries(davif PRIVATE pthread)
target_link_libraries(davif PRIVATE dl)

###############################################################################

install(TARGETS davif
    RUNTIME DESTINATION bin
)

