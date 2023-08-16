add_library(gstreamer_android SHARED gstreamer_android.c)
target_link_libraries(
	gstreamer_android
	PRIVATE
		${GST_LIBRARIES}
		-Wl,--whole-archive
		${GST_ARCH_DIR}/lib/libgstreamer-1.0.a
		-Wl,--no-whole-archive
		${JSONGLIB_LIBRARIES}
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
		PROPERTY
			LINK_FLAGS
			" -u gst_amc_jni_set_java_vm"
		)
