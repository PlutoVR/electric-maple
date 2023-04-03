# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(comp_pluto STATIC pl_comp.c pl_comp.h)
target_link_libraries(
	comp_pluto
	PUBLIC xrt-interfaces
	PRIVATE
		aux_util
		aux_os
		aux_vk
		comp_util
		comp_multi
	)
target_include_directories(comp_pluto PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(comp_pluto INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
