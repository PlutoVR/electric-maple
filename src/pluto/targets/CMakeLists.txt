add_executable(
	pluto_streaming_server pluto_streaming_server/main.cpp
			       pluto_streaming_server/pluto_instance.cpp
	)

target_link_libraries(
	pluto_streaming_server
	PRIVATE
		aux_util
		aux_util_debug_gui
		st_prober
		ipc_server
		# target_lists
		# target_instance
		comp_main
		drv_pluto
	)

target_link_libraries(pluto_streaming_server PRIVATE st_gui xrt-external-imgui-sdl2 aux_ogl)
