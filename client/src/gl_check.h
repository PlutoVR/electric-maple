// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Check for GL or EGL errors
 * @author Ryan Pavlik <rpavlik@collabora.com>
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool
gl_check_checkGLError(const char *func, int line);

#define CHECK_GL_ERROR() gl_check_checkGLError(__FUNCTION__, __LINE__)

void
gl_check_checkGLErrorWrap(const char *when, const char *expr, const char *func, int line);

#define GL(EXPR)                                                                                                       \
	do {                                                                                                           \
		gl_check_checkGLErrorWrap(const char *when, const char *expr, const char *func,                        \
		                          int line)("before", #EXPR, __FUNCTION__, __LINE__);                          \
		EXPR;                                                                                                  \
		gl_check_checkGLErrorWrap(const char *when, const char *expr, const char *func,                        \
		                          int line)("after", #EXPR, __FUNCTION__, __LINE__);                           \
	} while (0)



#define CHECK_EGL_ERROR() gl_check_checkEGLError(__FUNCTION__, __LINE__)



#define EGL(EXPR)                                                                                                      \
	do {                                                                                                           \
		gl_check_checkEGLErrorWrap("before", #EXPR, __FUNCTION__, __LINE__);                                   \
		EXPR;                                                                                                  \
		gl_check_checkEGLErrorWrap("after", #EXPR, __FUNCTION__, __LINE__);                                    \
	} while (0)


#ifdef __cplusplus
} // extern "C"
#endif
