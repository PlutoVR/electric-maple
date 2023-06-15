add_subdirectory(gst)

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(
	native_app_glue STATIC
	${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
	)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(plutovr_webrtc_client SHARED main.cpp render.cpp common.hpp)

target_include_directories(
	plutovr_webrtc_client
	PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue ${CMAKE_CURRENT_BINARY_DIR}/shaders/
		${CMAKE_CURRENT_SOURCE_DIR}/../external/include
		${ANDROID_NDK}/sources/android/native_app_glue
		${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include
		${GLIB_INCLUDE_DIRS}
		${GST_INCLUDE_DIRS}
	)

# add lib dependencies
target_link_libraries(
	plutovr_webrtc_client
		# VERY important here to have --whole-archive for libgstreamer-1.0 because gstandroid's gst_android_get_application_class_loader will be dlloaded by the amc plugin and if not, clang does not add those symbols.
		-Wl,--whole-archive ${gstreamer_android_SOURCE_DIR}/arm64/lib/libgstreamer-1.0.a -Wl,--no-whole-archive
		#/home/fredinfinite23/code/cerbero/build/sources/android_arm64/gstreamer-1.0-1.22.3/_builddir/gst/libgstreamer-1.0.so
	android
	OpenXR::openxr_loader
	Vulkan::Vulkan
	#vulkan
	native_app_glue
	EGL
	GLESv3
	android
	log # This is an android thing
	openxr_loader
	pluto_proto
	aux_util
		pluto_vf
		#-Wl,--whole-archive pluto_vf -Wl,--no-whole-archive
		# drv_includes
	# drv_vf
	# delete-me-nanopb
	)
