// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief  Helper for GL error checking
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_utils
 */

#include "GLError.h"

#include "../em_app_log.h"

#include <GLES3/gl3.h>
#include <EGL/egl.h>

bool
checkGLError(const char *func, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		ALOGE("RYLIE: %s:%d: OpenGL error: %d,", func, line, err);
		return false;
	}
	return true;
}
void
checkGLErrorWrap(const char *when, const char *expr, const char *func, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		ALOGE("RYLIE: %s:%d: OpenGL error %s call to %s: %d,", func, line, when, expr, err);
	}
}
bool
checkEGLError(const char *func, int line)
{
	EGLint err = eglGetError();
	if (err != EGL_SUCCESS) {
		ALOGE("RYLIE: %s:%d: EGL error: %d,", func, line, err);
		return false;
	}
	return true;
}

void
checkEGLErrorWrap(const char *when, const char *expr, const char *func, int line)
{
	EGLint err = eglGetError();
	if (err != EGL_SUCCESS) {
		ALOGE("RYLIE: %s:%d: EGL error %s call to %s: %d,", func, line, when, expr, err);
	}
}
