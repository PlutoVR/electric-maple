# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

add_library(em_egl STATIC em_egl.cpp)
target_link_libraries(
	em_egl
	PRIVATE ${ANDROID_LOG_LIBRARY}
	PUBLIC EGL::EGL OpenGLES::OpenGLESv3
	)
target_compile_definitions(em_egl PUBLIC XR_USE_PLATFORM_ANDROID XR_USE_GRAPHICS_API_OPENGL_ES)
target_include_directories(
	em_egl
	PUBLIC ..
	# just for the logging header
	PRIVATE ../../src/em
	)
