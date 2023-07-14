// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  EGL utilities originating in the Monado EGL client compositor codebase
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#pragma once

#include <EGL/egl.h>

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif
typedef __eglMustCastToProperFunctionPointerType (*PFNEGLGETPROCADDRESSPROC)(const char *proc);

const char *
eglutils_error_str(EGLint ret);

struct eglutils_context
{
	EGLDisplay dpy;
	EGLContext ctx;
	EGLSurface read, draw;
};

void
eglutils_context_save(struct eglutils_context *ctx);

bool
eglutils_context_restore(struct eglutils_context *ctx);

/*!
 * EGL based compositor, carries the extra needed EGL information needed by the
 * client side code and can handle both GL Desktop or GLES contexts.
 *
 * @ingroup comp_client
 */
struct eglutils_compositor
{
	// struct client_gl_compositor base;
	struct eglutils_context current, previous;
};

int
xrt_gfx_provider_create_gl_egl(EGLDisplay display,
                               EGLConfig config,
                               EGLContext context,
                               PFNEGLGETPROCADDRESSPROC get_gl_procaddr,
                               struct eglutils_compositor **out_xcgl);

void
eglutil_make_current(EGLDisplay display, EGLSurface surface, EGLContext context);

void
eglutil_release_current(EGLDisplay display);

#ifdef __cplusplus
} // extern "C"
#endif
