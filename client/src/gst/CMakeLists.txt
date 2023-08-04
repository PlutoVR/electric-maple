add_library(pluto_vf SHARED em_connection.c em_stream_client.c em_sample.c)
if(ANDROID)
	target_link_libraries(pluto_vf PRIVATE gstreamer_android ${ANDROID_LOG_LIBRARY})
	set_property(
		TARGET pluto_vf
		APPEND_STRING
		PROPERTY LINK_FLAGS " -u gst_amc_jni_set_java_vm -u JNI_OnLoad -u JNI_OnUnload"
		)
endif()
target_link_libraries(
	pluto_vf
	PRIVATE aux_util
	PUBLIC
		OpenXR::openxr_loader # actually only need headers but prefab doesn't expose that
		EGL::EGL
		OpenGLES::OpenGLESv3
		xrt-interfaces
		aux_os
	)
if(NOT ANDROID)

	target_link_libraries(
		pluto_vf
		PRIVATE
			${GST_LIBRARIES}
			${JSONGLIB_LIBRARIES}
			${LIBSOUP_LIBRARIES}
			${GIO_LIBRARIES}
			${GLIB_LIBRARIES}
		)

endif()
# target_compile_definitions(pluto_vf PUBLIC XR_USE_PLATFORM_ANDROID XR_USE_GRAPHICS_API_OPENGL_ES)
target_include_directories(
	pluto_vf
	PRIVATE
		${GST_INCLUDE_DIRS}
		${GLIB_INCLUDE_DIRS}
		${LIBSOUP_INCLUDE_DIRS}
		${JSONGLIB_INCLUDE_DIRS}
		${GIO_INCLUDE_DIRS}
	)
