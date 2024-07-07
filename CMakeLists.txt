# Copyright 2024 Zorxx Software. All rights reserved.
if(IDF_TARGET)
    idf_component_register(SRCS "lib/bmp180.c" "lib/bmp180_calculate.c" "lib/esp-idf.c"
                           INCLUDE_DIRS "lib" "include"
                           PRIV_INCLUDE_DIRS "lib" "include/bmp180"
                           PRIV_REQUIRES "driver" "esp_timer")
    return()
endif()

cmake_minimum_required(VERSION 3.5)
set(project bmp180)
project(${project} LANGUAGES C VERSION 1.3.0)

add_library(bmp180 STATIC lib/bmp180.c lib/bmp180_calculate.c lib/linux.c)
target_include_directories(bmp180 PUBLIC include)
target_include_directories(bmp180 PRIVATE lib include/bmp180)
install(TARGETS bmp180 LIBRARY DESTINATION lib)
install(DIRECTORY include/bmp180 DESTINATION include)

add_subdirectory(test)
add_subdirectory(example/linux)
