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

add_library(plutovr_webrtc_client SHARED main.cpp)

target_include_directories(
	plutovr_webrtc_client
	PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue ${CMAKE_CURRENT_BINARY_DIR}/shaders/
		${CMAKE_CURRENT_SOURCE_DIR}/../external/include
	)

# add lib dependencies
target_link_libraries(
	plutovr_webrtc_client
	android
	OpenXR::openxr_loader
	Vulkan::Vulkan
	native_app_glue
	EGL
	GLESv3
	android
	log # This is an android thing
	openxr_loader
	pluto_proto
	aux_util
	# delete-me-nanopb
	)
