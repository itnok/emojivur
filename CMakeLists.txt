#  ------------------------------------------------------------------------  #
#                        _ _                                                 #
#    ___ _ __ ___   ___ (_|_)_   ___   _ _ __                                #
#   / _ \ '_ ` _ \ / _ \| | \ \ / / | | | '__|                               #
#  |  __/ | | | | | (_) | | |\ V /| |_| | |                                  #
#   \___|_| |_| |_|\___// |_| \_/  \__,_|_|                                  #
#                     |__/                                                   #
#                                                                            #
#  ------------------------------------------------------------------------  #
#  emojivur                                                                  #
#  Lightweight emoji viewer and PDF conversion utility                       #
#  ------------------------------------------------------------------------  #
#  Copyright (c) 2020 Simone Conti, @itnok <s.conti@itnok.com>               #
#  All Rights Reserved.                                                      #
#                                                                            #
#  Distributed under MIT license.                                            #
#  See file LICENSE for detail                                               #
#  or copy at https://opensource.org/licenses/MIT                            #
#  ------------------------------------------------------------------------  #
#  \file       CMakeLists.txt
#  \author     Simone Conti (itnok)
#  \date       2020/05/16
#
#  \brief      Main cmake configuration file
#
cmake_minimum_required(VERSION 3.10)

# Prefer files from CMake module directory
cmake_policy(SET CMP0017 NEW)

# Enable MACOSX_RPATH by default
cmake_policy(SET CMP0042 NEW)

# Set built type to `Release` by default
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build" FORCE)
endif()

set(PROJECT_NAME emojivur)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
    "${CMAKE_BINARY_DIR}/config.h"
)

project(${PROJECT_NAME}
        LANGUAGES C
        VERSION ${PROJECT_VERSION})

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED 1)

# Add local CMake module directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(${CMAKE_MODULE_PATH}/gengetopt.cmake)

set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(PkgConfig)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(SDL2 REQUIRED sdl2 sdl2_image)

set(BUILD_FLAGS "-Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${BUILD_FLAGS}")

add_compile_definitions(HAVE_CONFIG_H)

include_directories("/usr/include" "/usr/local/include")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(${PROJECT_NAME}_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(${PROJECT_NAME}_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.c)
add_gengetopt_files(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/cli_options.ggo)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRC})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_GENGETOPT_FILES)
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIR})

# CAIRO
if(CAIRO_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CAIRO_LIBRARIES})
    get_filename_component(CAIRO_LIBRARY_DIR ${pkgcfg_lib_CAIRO_cairo} DIRECTORY)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CAIRO_LIBRARY_DIR})
    target_include_directories(${PROJECT_NAME} PUBLIC ${CAIRO_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME} PUBLIC ${CAIRO_CFLAGS_OTHER})
endif()

# Harfbuzz
if(HARFBUZZ_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${HARFBUZZ_LIBRARIES})
    get_filename_component(HARFBUZZ_LIBRARY_DIR ${pkgcfg_lib_HARFBUZZ_harfbuzz} DIRECTORY)
    target_link_directories(${PROJECT_NAME} PRIVATE ${HARFBUZZ_LIBRARY_DIR})
    target_include_directories(${PROJECT_NAME} PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME} PUBLIC ${HARFBUZZ_CFLAGS_OTHER})
endif()

# Freetype2
if(FREETYPE_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FREETYPE_LIBRARIES})
    get_filename_component(FREETYPE_LIBRARY_DIR ${pkgcfg_lib_FREETYPE_freetype} DIRECTORY)
    target_link_directories(${PROJECT_NAME} PRIVATE ${FREETYPE_LIBRARY_DIR})
    target_include_directories(${PROJECT_NAME} PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME} PUBLIC ${FREETYPE_CFLAGS_OTHER})
endif()

# SDL2 & SDL2_Image
if(SDL2_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
    get_filename_component(SDL2_LIBRARY_DIR ${pkgcfg_lib_SDL2_SDL2} DIRECTORY)
    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY_DIR})
    target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME} PUBLIC ${SDL2_CFLAGS_OTHER})
endif()

# Setup files to be installed (DO NOT install all dependencies!)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
