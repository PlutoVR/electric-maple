// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  OpenGL ES debug callback
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_utils
 */

#include "GLDebug.h"

#include "gst/app_log.h"

#include "GLES3/gl3.h"
#include <GLES3/gl32.h>
#include <android/log.h>

namespace {

#define MAKE_CASE(ENUM)                                                                                                \
	case (ENUM): return #ENUM
const char *
glDebugSourceToString(GLenum e)
{
	switch (e) {

		MAKE_CASE(GL_DEBUG_SOURCE_API);
		MAKE_CASE(GL_DEBUG_SOURCE_WINDOW_SYSTEM);
		MAKE_CASE(GL_DEBUG_SOURCE_SHADER_COMPILER);
		MAKE_CASE(GL_DEBUG_SOURCE_THIRD_PARTY);
		MAKE_CASE(GL_DEBUG_SOURCE_APPLICATION);
		MAKE_CASE(GL_DEBUG_SOURCE_OTHER);

	default: return "unknown-source";
	}
}

const char *
glDebugTypeToString(GLenum e)
{
	switch (e) {

		MAKE_CASE(GL_DEBUG_TYPE_ERROR);
		MAKE_CASE(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
		MAKE_CASE(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
		MAKE_CASE(GL_DEBUG_TYPE_PORTABILITY);
		MAKE_CASE(GL_DEBUG_TYPE_PERFORMANCE);
		MAKE_CASE(GL_DEBUG_TYPE_OTHER);
		MAKE_CASE(GL_DEBUG_TYPE_MARKER);
		MAKE_CASE(GL_DEBUG_TYPE_PUSH_GROUP);
		MAKE_CASE(GL_DEBUG_TYPE_POP_GROUP);

	default: return "unknown-type";
	}
}
#undef MAKE_CASE

android_LogPriority
glDebugSeverityToAndroidLogPriority(GLenum e)
{

	switch (e) {
	case GL_DEBUG_SEVERITY_HIGH: return ANDROID_LOG_ERROR;
	case GL_DEBUG_SEVERITY_MEDIUM: return ANDROID_LOG_WARN;
	case GL_DEBUG_SEVERITY_LOW: return ANDROID_LOG_INFO;
	case GL_DEBUG_SEVERITY_NOTIFICATION: return ANDROID_LOG_DEBUG;
	default: return ANDROID_LOG_VERBOSE;
	}
}

void KHRONOS_APIENTRY
gl_debug_callback(GLenum source,
                  GLenum type,
                  GLuint id,
                  GLenum severity,
                  GLsizei /* length */,
                  const GLchar *message,
                  const void *userParam)
{
	__android_log_print(glDebugSeverityToAndroidLogPriority(severity), LOG_TAG, "GL: %s: %s (id %d): %s",
	                    glDebugSourceToString(source), glDebugTypeToString(type), id, message);
}

} // namespace

void
registerGlDebugCallback()
{
	glDebugMessageCallback(&gl_debug_callback, nullptr);
	glEnable(GL_DEBUG_OUTPUT);
}
