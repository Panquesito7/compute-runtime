#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

target_sources(igdrcl_aub_tests PRIVATE
               ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
               ${CMAKE_CURRENT_SOURCE_DIR}/command_enqueue_fixture.h
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_copy_buffer_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_copy_buffer_rect_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_copy_image_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_fill_buffer_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_fill_image_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_kernel_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_map_buffer_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_map_image_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_printf_kernel_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_read_buffer_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_read_buffer_rect_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_read_image_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_read_write_image_aub_fixture.h
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_verify_memory_buffer_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_verify_memory_image_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_write_buffer_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_write_buffer_rect_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_write_copy_read_buffer_aub_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_write_copy_read_buffer_aub_tests.h
               ${CMAKE_CURRENT_SOURCE_DIR}/enqueue_write_image_aub_tests.cpp
)

add_subdirectories()
