# Copyright 2024 Zorxx Software. All rights reserved.
add_executable(test main.c)
target_link_libraries(test bmp180)
target_compile_definitions(test PRIVATE SYS_DEBUG_ENABLE)
target_include_directories(test PRIVATE ../lib ../include/bmp180)
