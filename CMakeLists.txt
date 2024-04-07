# Copyright 2024 Zorxx Software. All rights reserved.
if(IDF_TARGET)
    idf_component_register(SRCS "bmp180.c" "bmp180_calculate.c"
                           INCLUDE_DIRS "." "include"
                           PRIV_REQUIRES "driver")
    return()
endif()

cmake_minimum_required(VERSION 3.5)
set(project bmp180)
project(${project} LANGUAGES C VERSION 1.0.0)

add_library(bmp180 STATIC bmp180_calculate.c)
set_target_properties(bmp180 PROPERTIES PUBLIC_HEADER bmp180.h)
target_include_directories(bmp180 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS bmp180
        LIBRARY DESTINATION $(CMAKE_INSTALL_LIBDIR)
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCDIR})

add_subdirectory(test)
