#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if(TESTS_XEHP)
  set(IGDRCL_SRCS_tests_xe_hp_core_xehp
      ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
      ${CMAKE_CURRENT_SOURCE_DIR}/cache_flush_tests_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/get_device_info_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/hw_helper_tests_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/hw_info_tests_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/memory_manager_tests_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/sampler_tests_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/source_level_debugger_csr_tests_xehp.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test_command_stream_receiver_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/test_device_caps_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/test_hw_info_config_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/test_local_work_size_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/test_sub_devices_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/test_preamble_xehp.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test_platform_caps_xehp.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/test_wrapper_xehp.cpp
  )

  target_sources(igdrcl_tests PRIVATE ${IGDRCL_SRCS_tests_xe_hp_core_xehp})
  add_subdirectories()
  neo_copy_test_files_with_revision(copy_test_files_xehp_0 xehp 0)
  add_dependencies(copy_test_files_per_product copy_test_files_xehp_0)
endif()
