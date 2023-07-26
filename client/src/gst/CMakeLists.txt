add_library(pluto_vf SHARED gst_driver.c em_connection.c)
target_link_libraries(
	pluto_vf
	PRIVATE
		gstreamer_android
		aux_util
		${JSONGLIB_LIBRARIES}
		${LIBSOUP_LIBRARIES}
		${GIO_LIBRARIES}
		${GLIB_LIBRARIES}
		${ANDROID_LOG_LIBRARY}
	PUBLIC
		OpenXR::openxr_loader # actually only need headers but prefab doesn't expose that
		EGL::EGL
		OpenGLES::OpenGLESv3
		xrt-interfaces
		aux_os
	)
# target_compile_definitions(pluto_vf PUBLIC XR_USE_PLATFORM_ANDROID XR_USE_GRAPHICS_API_OPENGL_ES)
target_include_directories(
	pluto_vf
	PRIVATE
		${GST_INCLUDE_DIRS}
		${GLIB_INCLUDE_DIRS}
		${LIBSOUP_INCLUDE_DIRS}
		${JSONGLIB_INCLUDE_DIRS}
		${GIO_INCLUDE_DIRS}
	PUBLIC ${PROJECT_SOURCE_DIR}/gstreamer-util
	)
