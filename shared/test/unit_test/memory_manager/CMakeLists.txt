#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

target_sources(${TARGET_NAME} PRIVATE
               ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
               ${CMAKE_CURRENT_SOURCE_DIR}/alignment_selector_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/multi_graphics_allocation_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/special_heap_pool_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/address_mapper_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/graphics_allocation_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/local_memory_usage_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/memory_pool_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/deferrable_allocation_deletion_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/deferred_deleter_mt_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/${BRANCH_DIR_SUFFIX}/gfx_partition_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/gfx_partition_tests.inl
               ${CMAKE_CURRENT_SOURCE_DIR}/page_table_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/physical_address_allocator_tests.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/physical_address_allocator_hw_tests.cpp
)

add_subdirectories()
