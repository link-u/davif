cmake_minimum_required(VERSION 3.13)
project(davif)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ENABLE_DOCS OFF)

if (WIN32)
  # https://stackoverflow.com/questions/47690822/possible-to-force-cmake-msvc-to-use-utf-8-encoding-for-source-files-without-a-bo
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

  # https://stackoverflow.com/a/60410369
  set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/_deps/lib/pkgconfig\;$ENV{PKG_CONFIG_PATH}")
else(WIN32)
  set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/_deps/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif(WIN32)

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
# pkg-config path for dependencies
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/_deps/include")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/_deps/lib")
find_package(PkgConfig)
pkg_check_modules(DAV1D REQUIRED dav1d)
###############################################################################

add_executable(davif
    src/img/Conversion.cpp
    src/img/Conversion.hpp

    src/img/png/Writer.cpp
    src/img/png/Writer.hpp

    src/main.cpp
)

# https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html#requiring-language-standards
# https://stackoverflow.com/questions/45688522/how-to-enable-c17-in-cmake
target_compile_features(davif PRIVATE cxx_std_17)
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set_property(TARGET davif PROPERTY CXX_STANDARD 17)
set_property(TARGET davif PROPERTY CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set_property(TARGET davif PROPERTY CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")
endif()

target_include_directories(davif PRIVATE external/libpng)
target_include_directories(davif PRIVATE "${CMAKE_BINARY_DIR}/include")
target_include_directories(davif PRIVATE external/libavif-container/src)
target_include_directories(davif PRIVATE external/libyuv/include)
target_include_directories(davif PRIVATE ${DAV1D_INCLUDE_DIRS})

target_link_directories(davif PRIVATE "${CMAKE_BINARY_DIR}/lib")
target_link_libraries(davif PRIVATE libavif-container)
target_link_libraries(davif PRIVATE fmt::fmt)
target_link_libraries(davif PRIVATE zlibstatic)
target_link_libraries(davif PRIVATE png_static)
target_link_libraries(davif PRIVATE pthread)
target_link_libraries(davif PRIVATE dl)
target_link_libraries(davif PRIVATE ${DAV1D_LDFLAGS} ${DAV1D_LIBRARIES})

###############################################################################

install(TARGETS davif
    RUNTIME DESTINATION bin
)
