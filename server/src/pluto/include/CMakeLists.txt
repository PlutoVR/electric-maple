# Copyright 2023, Pluto VR, Inc.

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/pl_build.h.cmake_in ${CMAKE_CURRENT_BINARY_DIR}/pl_build.h
	@ONLY
	)

add_library(pl_build_defines INTERFACE)

target_include_directories(pl_build_defines INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
