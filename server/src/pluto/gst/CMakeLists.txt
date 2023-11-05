add_library(pl_gst STATIC gst_webrtc_pipeline.c gst_webrtc_server.c)

target_link_libraries(
	pl_gst
	PRIVATE
		pl_build_defines
		pl_callbacks
		pluto_proto
		aux_util
		aux_gstreamer
		${GST_LIBRARIES}
		${GST_SDP_LIBRARIES}
		${GST_RTP_LIBRARIES}
		${GST_WEBRTC_LIBRARIES}
		${GLIB_LIBRARIES}
		${LIBSOUP_LIBRARIES}
		${JSONGLIB_LIBRARIES}
		${GIO_LIBRARIES}
	)

target_include_directories(
	pl_gst
	PRIVATE
		${GLIB_INCLUDE_DIRS}
		${GST_INCLUDE_DIRS}
		${LIBSOUP_INCLUDE_DIRS}
		${JSONGLIB_INCLUDE_DIRS}
		${GIO_INCLUDE_DIRS}
	)
target_compile_definitions(pl_gst PUBLIC G_LOG_DOMAIN="ElectricMapleServer")
