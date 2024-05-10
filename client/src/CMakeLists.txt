# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

add_subdirectory(em)

# build native_app_glue as a static lib
add_library(
	native_app_glue STATIC
	${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
	)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_library(electricmaple_standalone_client SHARED main.cpp EglData.cpp)

target_include_directories(
	electricmaple_standalone_client PRIVATE "${PROJECT_SOURCE_DIR}/../monado/external/include"
						${GLIB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS}
	)

# add lib dependencies
target_link_libraries(
	electricmaple_standalone_client
	PRIVATE
		em_proto
		aux_util
		electricmaple_client
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
	TARGET electricmaple_standalone_client
	APPEND_STRING
	PROPERTY LINK_FLAGS " -u ANativeActivity_onCreate"
	)
