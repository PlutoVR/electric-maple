add_subdirectory(gst)

# build native_app_glue as a static lib
add_library(
	native_app_glue STATIC
	${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
	)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(plutovr_webrtc_client SHARED main.cpp render.cpp eglutils.c gl_check.cpp)

target_include_directories(
	plutovr_webrtc_client
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/shaders/
		${PROJECT_SOURCE_DIR}/../monado/external/include ${GLIB_INCLUDE_DIRS}
		${GST_INCLUDE_DIRS}
	)

# add lib dependencies
target_link_libraries(
	plutovr_webrtc_client
	PRIVATE
		# VERY important here to have --whole-archive for libgstreamer-1.0 because gstandroid's gst_android_get_application_class_loader will be dlloaded by the amc plugin and if not, clang does not add those symbols.
		-Wl,--whole-archive
		${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstreamer-1.0.a
		-Wl,--no-whole-archive
		android
		OpenXR::openxr_loader
		Vulkan::Vulkan
		native_app_glue
		EGL
		GLESv3
		android
		log # This is an android thing
		pluto_proto
		aux_util
		pluto_vf
		#-Wl,--whole-archive pluto_vf -Wl,--no-whole-archive
		# drv_includes
		# drv_vf
		# delete-me-nanopb
	)
