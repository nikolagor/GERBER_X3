#/*******************************************************************************
#* Author    :  Damir Bakiev                                                    *
#* Version   :  na                                                              *
#* Date      :  11 November 2021                                                *
#* Website   :  na                                                              *
#* Copyright :  Damir Bakiev 2016-2022                                          *
#* License:                                                                     *
#* Use, modification & distribution is subject to Boost Software License Ver 1. *
#* http://www.boost.org/LICENSE_1_0.txt                                         *
#*******************************************************************************/
cmake_minimum_required(VERSION 3.16)

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")

project(GERBER_X3)

add_definitions(-DQT_MESSAGELOGCONTEXT)# qDebug()
add_definitions(-DCTRE)

set_property(GLOBAL PROPERTY CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Winvalid-pch)
    add_compile_options(-H)
    add_compile_options(-ftime-report)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()

include_directories(magic_get/)
include_directories(magic_get/include/)
include_directories(compile-time-regular-expressions/single-header/)


# static lib
add_subdirectory(static_libs)

# main app
add_subdirectory(GGEasy)

# plugins file
add_subdirectory(file_plugins)

# plugins shape
add_subdirectory(shape_plugins)


target_precompile_headers(gi
    #    PUBLIC
    #    "myclipper.h"
    #    project_header.h
    PRIVATE
    "static_libs/tooldatabase/toolpch.h"
    )

target_precompile_headers(thermal REUSE_FROM gi)
target_precompile_headers(GGEasy REUSE_FROM gi)
