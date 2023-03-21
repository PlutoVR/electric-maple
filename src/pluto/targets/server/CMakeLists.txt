# add_subdirectory(client)
# add_subdirectory(server)

add_executable(pluto_streaming_server pl_instance.cpp pl_server_internal.h pl_server_main.cpp)

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
