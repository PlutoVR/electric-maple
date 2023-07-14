// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  EGL utilities originating in the Monado EGL client compositor codebase
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @author Drew DeVault <sir@cmpwn.com>
 * @author Simon Ser <contact@emersion.fr>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include "eglutils.h"
#include "gst/app_log.h"
#include "xrt/xrt_compiler.h"

#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <stdbool.h>
#include <string.h>

#define EGL_TRACE(FMT, ...) ALOGV("[eglutils] " FMT, __VA_ARGS__)
#define EGL_DEBUG(FMT, ...) ALOGD("[eglutils] " FMT, __VA_ARGS__)
#define EGL_INFO(FMT, ...) ALOGI("[eglutils] " FMT, __VA_ARGS__)
#define EGL_WARN(FMT, ...) ALOGW("[eglutils] " FMT, __VA_ARGS__)
#define EGL_ERROR(FMT, ...) ALOGE("[eglutils] " FMT, __VA_ARGS__)


/*
 *
 * Declarations.
 *
 */
#ifdef XRT_OS_ANDROID
typedef const char *EGLAPIENTRY (*PFNEGLQUERYSTRINGIMPLEMENTATIONANDROIDPROC)(EGLDisplay dpy, EGLint name);
#endif

// Not forward declared by mesa
typedef EGLBoolean
    EGLAPIENTRY (*PFNEGLMAKECURRENTPROC)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);

/*
 *
 * Old helper.
 *
 */

void
eglutils_context_save(struct eglutils_context *ctx)
{
	ctx->dpy = eglGetCurrentDisplay();
	ctx->ctx = EGL_NO_CONTEXT;
	ctx->read = EGL_NO_SURFACE;
	ctx->draw = EGL_NO_SURFACE;

	if (ctx->dpy != EGL_NO_DISPLAY) {
		ctx->ctx = eglGetCurrentContext();
		ctx->read = eglGetCurrentSurface(EGL_READ);
		ctx->draw = eglGetCurrentSurface(EGL_DRAW);
	}
}

bool
eglutils_context_restore(struct eglutils_context *ctx)
{
	/* We're using the current display if we're trying to restore a null context */
	EGLDisplay dpy = ctx->dpy == EGL_NO_DISPLAY ? eglGetCurrentDisplay() : ctx->dpy;

	if (dpy == EGL_NO_DISPLAY) {
		/* If the current display is also null then the call is a no-op */
		return true;
	}

	return eglMakeCurrent(dpy, ctx->draw, ctx->read, ctx->ctx);
}


/*
 *
 * Helper functions.
 *
 */

const char *
eglutils_error_str(EGLint ret)
{
	switch (ret) {
	case EGL_SUCCESS: return "EGL_SUCCESS";
	case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
	case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
	case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
	case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
	case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
	case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
	case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
	case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
	case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
	case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
	case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
	case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
	case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
	case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
	default: return "EGL_<UNKNOWN>";
	}
}

static inline void
destroy_context_with_check(EGLDisplay display, EGLContext context, const char *func)
{
	EGLBoolean eret = eglDestroyContext(display, context);
	if (eret == EGL_FALSE) {
		EGL_ERROR("eglDestroyContext: %s (%s)", eglutils_error_str(eglGetError()), func);
	}
}

#define DESTROY_CONTEXT(DPY, CTX) destroy_context_with_check(DPY, CTX, __func__)

XRT_MAYBE_UNUSED static bool
has_extension(const char *extensions, const char *ext)
{
	const char *loc = NULL;
	const char *terminator = NULL;

	if (extensions == NULL) {
		return false;
	}

	while (1) {
		loc = strstr(extensions, ext);
		if (loc == NULL) {
			return false;
		}

		terminator = loc + strlen(ext);
		if ((loc == extensions || *(loc - 1) == ' ') && (*terminator == ' ' || *terminator == '\0')) {
			return true;
		}
		extensions = terminator;
	}
}


/*
 *
 * Creation helper functions.
 *
 */

static void
ensure_native_fence_is_loaded(EGLDisplay dpy, PFNEGLGETPROCADDRESSPROC get_gl_procaddr)
{
#ifdef XRT_OS_ANDROID
	// clang-format off
	PFNEGLQUERYSTRINGIMPLEMENTATIONANDROIDPROC eglQueryStringImplementationANDROID;
	// clang-format on

	eglQueryStringImplementationANDROID =
	    (PFNEGLQUERYSTRINGIMPLEMENTATIONANDROIDPROC)get_gl_procaddr("eglQueryStringImplementationANDROID");

	// On Android, EGL_ANDROID_native_fence_sync only shows up in this
	// extension list, not the normal one.
	const char *ext = eglQueryStringImplementationANDROID(dpy, EGL_EXTENSIONS);
	if (!has_extension(ext, "EGL_ANDROID_native_fence_sync")) {
		return;
	}

	GLAD_EGL_ANDROID_native_fence_sync = true;
	glad_eglDupNativeFenceFDANDROID =
	    (PFNEGLDUPNATIVEFENCEFDANDROIDPROC)get_gl_procaddr("eglDupNativeFenceFDANDROID");
#endif
}

static int
create_context(
    EGLDisplay display, EGLConfig config, EGLContext app_context, EGLint api_type, EGLContext *out_our_context)
{
	EGLint old_api_type = eglQueryAPI();

	eglBindAPI(api_type);

	// clang-format off
	EGLint attrs[] = {
	    EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
	    EGL_CONTEXT_MINOR_VERSION_KHR, 1, // Panfrost only supports 3.1
	    EGL_NONE, EGL_NONE,
	    EGL_NONE,
	};
	// clang-format on

	if (api_type == EGL_OPENGL_API) {
		attrs[4] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
		attrs[5] = EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT;
	}

	EGLContext our_context = eglCreateContext(display, config, app_context, attrs);

	// Restore old API type.
	if (old_api_type == EGL_NONE) {
		eglBindAPI(old_api_type);
	}

	if (our_context == EGL_NO_CONTEXT) {
		EGL_ERROR("eglCreateContext: %s", eglutils_error_str(eglGetError()));
		return -1;
	}

	*out_our_context = our_context;

	return 0;
}


static void
check_context_and_debug_print(EGLint egl_client_type)
{
	EGL_DEBUG(                    //
	    "OpenGL context:"         //
	    "\n\tGL_VERSION: %s"      //
	    "\n\tGL_RENDERER: %s"     //
	    "\n\tGL_VENDOR: %s",      //
	    glGetString(GL_VERSION),  //
	    glGetString(GL_RENDERER), //
	    glGetString(GL_VENDOR));  //
}
