// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Check for GL or EGL errors
 * @author Ryan Pavlik <rpavlik@collabora.com>
 */

#include "gl_check.h"
#include "gst/app_log.h"


#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define GLC_TRACE(FMT, ...) ALOGV("[gl_check] " FMT, __VA_ARGS__)
#define GLC_DEBUG(FMT, ...) ALOGD("[gl_check] " FMT, __VA_ARGS__)
#define GLC_INFO(FMT, ...) ALOGI("[gl_check] " FMT, __VA_ARGS__)
#define GLC_WARN(FMT, ...) ALOGW("[gl_check] " FMT, __VA_ARGS__)
#define GLC_ERROR(FMT, ...) ALOGE("[gl_check] " FMT, __VA_ARGS__)

bool
gl_check_checkGLError(const char *func, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		GLC_ERROR("OpenGL error: %d, %s:%d", err, func, line);
		return false;
	}
	return true;
}

void
gl_check_checkGLErrorWrap(const char *when, const char *expr, const char *func, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		GLC_ERROR("OpenGL error: %s %s: %d, %s:%d", when, expr, err, func, line);
	}
}

bool
gl_check_checkEGLError(const char *func, int line)
{
	EGLint err = eglGetError();
	if (err != EGL_SUCCESS) {
		GLC_ERROR("EGL error: %d, %s:%d", err, func, line);
		return false;
	}
	return true;
}

void
gl_check_checkEGLErrorWrap(const char *when, const char *expr, const char *func, int line)
{
	EGLint err = eglGetError();
	if (err != EGL_SUCCESS) {
		GLC_ERROR("EGL error: %s %s: %d, %s:%d", when, expr, err, func, line);
	}
}
