#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(IGDRCL_SRCS_tests_gen12_rkl_linux
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/hw_info_config_tests_rkl.cpp
)
if(UNIX)
  target_sources(igdrcl_tests PRIVATE ${IGDRCL_SRCS_tests_gen12_rkl_linux})
  add_subdirectory(dll)
endif()
