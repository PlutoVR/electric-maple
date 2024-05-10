# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

add_executable(webrtc_client webrtc_client.c)

target_link_libraries(
	webrtc_client
	PRIVATE
		ems_build_defines
		aux_util
		aux_gstreamer
		${GST_LIBRARIES}
		${GST_SDP_LIBRARIES}
		${GST_WEBRTC_LIBRARIES}
		${GLIB_LIBRARIES}
		${LIBSOUP_LIBRARIES}
		${JSONGLIB_LIBRARIES}
		${GIO_LIBRARIES}
	)

target_include_directories(
	webrtc_client
	PRIVATE
		${GLIB_INCLUDE_DIRS}
		${GST_INCLUDE_DIRS}
		${LIBSOUP_INCLUDE_DIRS}
		${JSONGLIB_INCLUDE_DIRS}
		${GIO_INCLUDE_DIRS}
	)
