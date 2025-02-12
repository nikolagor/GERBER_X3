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

project(shape LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets OpenGL REQUIRED)

include_directories(
    ../../GGEasy
    ../../static_libs/clipper
    ../../static_libs/common
    ../../static_libs/filetree
    ../../static_libs/gerber
    ../../static_libs/gi
    ../../static_libs/graphicsview
    ../../static_libs/thermal
    ../../static_libs/tooldatabase
    ../../static_libs/tooldatabase
    )

add_library(shape STATIC
    shape.cpp
    shape.h
    shhandler.cpp
    shhandler.h
    shheaders.h
    shnode.cpp
    shnode.h
    )

target_link_libraries(shape PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    )

#install(TARGETS ${PROJECT_NAME} DESTINATION ${PLUGINS_DIR})

#target_compile_definitions(shape PRIVATE SHAPE_LIBRARY)
