# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/ems_build.h.cmake_in ${CMAKE_CURRENT_BINARY_DIR}/ems_build.h
	@ONLY
	)

add_library(ems_build_defines INTERFACE)

target_include_directories(ems_build_defines INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
