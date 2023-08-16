// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Helper for GL error checking
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_utils
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool
checkGLError(const char *func, int line);

#define CHECK_GL_ERROR() checkGLError(__FUNCTION__, __LINE__)

void
checkGLErrorWrap(const char *when, const char *expr, const char *func, int line);

#define CHK_GL(EXPR)                                                                                                   \
	do {                                                                                                           \
		checkGLErrorWrap("before", #EXPR, __FUNCTION__, __LINE__);                                             \
		EXPR;                                                                                                  \
		checkGLErrorWrap("after", #EXPR, __FUNCTION__, __LINE__);                                              \
	} while (0)


bool
checkEGLError(const char *func, int line);

#define CHECK_EGL_ERROR() checkEGLError(__FUNCTION__, __LINE__)

void
checkEGLErrorWrap(const char *when, const char *expr, const char *func, int line);

#define CHK_EGL(EXPR)                                                                                                  \
	do {                                                                                                           \
		checkEGLErrorWrap("before", #EXPR, __FUNCTION__, __LINE__);                                            \
		EXPR;                                                                                                  \
		checkEGLErrorWrap("after", #EXPR, __FUNCTION__, __LINE__);                                             \
	} while (0)

#ifdef __cplusplus
} // extern "C"
#endif
