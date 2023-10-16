# Copyright 2019-2023, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_subdirectory(include)

add_library(pl_callbacks STATIC pl_callbacks.cpp)
target_link_libraries(
	pl_callbacks
	PUBLIC xrt-interfaces
	PRIVATE aux_util
	)

target_include_directories(pl_callbacks PUBLIC . ${GLIB_INCLUDE_DIRS})

add_subdirectory(gst)

add_library(comp_pluto STATIC pl_comp.cpp pl_comp.h)
target_link_libraries(
	comp_pluto
	PUBLIC xrt-interfaces
	PRIVATE
		aux_util
		aux_os
		aux_vk
		comp_util
		comp_multi
		ems_gst
	)
target_include_directories(comp_pluto PUBLIC . ${GST_INCLUDE_DIRS})

add_library(drv_pluto STATIC pl_driver.cpp pl_device_controller.cpp)

target_link_libraries(drv_pluto PRIVATE xrt-interfaces aux_util pluto_proto pl_callbacks)

add_executable(pluto_streaming_server pl_instance.cpp pl_server_internal.h pl_server_main.cpp)

target_link_libraries(
	pluto_streaming_server
	PRIVATE
		aux_util
		aux_util_debug_gui
		st_prober
		ipc_server
		comp_main
		drv_pluto
		pluto_proto
		comp_pluto
		pl_callbacks
	)

target_link_libraries(pluto_streaming_server PRIVATE st_gui xrt-external-imgui-sdl2 aux_ogl)
