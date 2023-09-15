add_library(gstreamer_android SHARED gstreamer_android.c)
target_link_libraries(
	gstreamer_android
	PRIVATE
		${GST_LIBRARIES}
		-Wl,--whole-archive
		${GST_ARCH_DIR}/lib/libgstreamer-1.0.a
		-Wl,--no-whole-archive
		${JSONGLIB_LIBRARIES}
		${LIBSOUP_LIBRARIES}
		EGL::EGL
	PUBLIC ${ANDROID_LOG_LIBRARY} z
	)
target_include_directories(
	gstreamer_android PRIVATE ${GST_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${JSONGLIB_INCLUDE_DIRS}
				  ${GIO_INCLUDE_DIRS}
	)
set_property(
	TARGET gstreamer_android
	APPEND_STRING
	PROPERTY LINK_FLAGS " -u gst_amc_jni_set_java_vm"
	)

# These are the symbols we must manually tell the shared libray to expose
set(symbols
    gst_amc_jni_set_java_vm
    json_builder_add_int_value
    json_builder_add_string_value
    json_builder_begin_object
    json_builder_end_object
    json_builder_get_root
    json_builder_new
    json_builder_set_member_name
    json_node_get_object
    json_node_unref
    json_parser_get_root
    json_parser_load_from_data
    json_parser_new
    json_to_string
    soup_message_new
    soup_session_get_type
    soup_session_new
    soup_session_websocket_connect_async
    soup_session_websocket_connect_finish
    soup_websocket_connection_send_text
    _SOUP_METHOD_GET
	)

foreach(symbol ${symbols})
	set_property(
		TARGET gstreamer_android
		APPEND_STRING
		PROPERTY LINK_FLAGS " -u ${symbol}"
		)
endforeach()
