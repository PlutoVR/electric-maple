add_library(pl_gst_rtp STATIC gstrtphdrext-electricmaple.c)

target_link_libraries(
	pl_gst_rtp
	PRIVATE
		pl_build_defines
		pluto_proto
		${GST_LIBRARIES}
		${GST_SDP_LIBRARIES}
		${GST_RTP_LIBRARIES}
		${GLIB_LIBRARIES}
		${GIO_LIBRARIES}
	)

target_include_directories(
	pl_gst_rtp
	PRIVATE
		${GLIB_INCLUDE_DIRS}
		${GST_INCLUDE_DIRS}
		${GIO_INCLUDE_DIRS}
	)
