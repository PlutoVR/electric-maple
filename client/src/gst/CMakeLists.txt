add_library(pluto_vf STATIC gst_driver.c)
target_link_libraries(
	pluto_vf
	PRIVATE
		${GST_LIBRARIES}
		# -Wl,--whole-archive
		# ${GST_ARCH_DIR}/lib/libgstreamer-1.0.a
		# -Wl,--no-whole-archive
		${JSONGLIB_LIBRARIES}
		EGL::EGL
	PUBLIC
		OpenXR::openxr_loader
		xrt-interfaces
		aux_os
		aux_util
		pluto_proto
	)
# target_compile_definitions(pluto_vf PUBLIC XR_USE_PLATFORM_ANDROID XR_USE_GRAPHICS_API_OPENGL_ES)
target_include_directories(
	pluto_vf
	PRIVATE
		${ANDROID_NDK}/sources/android/native_app_glue
		${CMAKE_CURRENT_SOURCE_DIR}/../monado/src/xrt/auxiliary/gstreamer
		${GST_INCLUDE_DIRS}
		${GLIB_INCLUDE_DIRS}
		${LIBSOUP_INCLUDE_DIRS}
		${JSONGLIB_INCLUDE_DIRS}
		${GIO_INCLUDE_DIRS}
	PUBLIC ${PROJECT_SOURCE_DIR}/gstreamer-util
	)
set_property(
		TARGET pluto_vf
		APPEND_STRING
		PROPERTY
			LINK_FLAGS
			"-u gst_android_get_application_class_loader"
		)
