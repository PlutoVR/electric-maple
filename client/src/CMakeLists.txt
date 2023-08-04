add_subdirectory(gst)
if(NOT ANDROID)
	return()
endif()

# build native_app_glue as a static lib
add_library(
	native_app_glue STATIC
	${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
	)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_library(plutovr_webrtc_client SHARED main.cpp render.cpp)

target_include_directories(
	plutovr_webrtc_client
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/shaders/
		"${PROJECT_SOURCE_DIR}/../monado/external/include" ${GLIB_INCLUDE_DIRS}
		${GST_INCLUDE_DIRS}
	)

# add lib dependencies
target_link_libraries(
	plutovr_webrtc_client
	PRIVATE
		pluto_proto
		aux_util
		pluto_vf
		gstreamer_android
		native_app_glue
		OpenXR::openxr_loader
		Vulkan::Vulkan
		EGL::EGL
		OpenGLES::OpenGLESv3
		${ANDROID_LIBRARY}
		${ANDROID_LOG_LIBRARY}
	)

# Export ANativeActivity_onCreate()
set_property(
	TARGET plutovr_webrtc_client
	APPEND_STRING
	PROPERTY LINK_FLAGS " -u ANativeActivity_onCreate"
	)
