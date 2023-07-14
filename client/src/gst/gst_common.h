// Copyright 2020-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup drv_vf
 */

#pragma once

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES

#include "xrt/xrt_frame.h"
#include "util/u_logging.h"
#include "util/u_time.h"
#include "os/os_time.h"

#include <android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <jni.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "pluto.pb.h"
#include "pb_encode.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdbool.h>

#define XR_LOAD(fn) xrGetInstanceProcAddr(state.instance, #fn, (PFN_xrVoidFunction *)&fn);

#include "xrt/xrt_frameserver.h"
#include "gstjniutils.h"

struct state_t
{
	struct android_app *app;
	JNIEnv *jni;
	JavaVM *java_vm;
	bool hasPermissions;
	// ANativeWindow *window; // Are we going to need this ?

	EGLDisplay display;
	// context created in initializeEGL
	EGLContext context;
	// config used to create context
	EGLConfig config;
	// 16x16 pbuffer surface
	EGLSurface surface;
	XrInstance instance;
	XrSystemId system;
	XrSession session;
	XrSessionState sessionState;
	XrSpace worldSpace;
	XrSpace viewSpace;
	XrSwapchain swapchain;
	XrSwapchainImageOpenGLESKHR images[4];
	GLuint framebuffers[4];
	GLuint shader_program;
	uint32_t imageCount;
	uint32_t width;
	uint32_t height;

	int socket_fd;
	struct sockaddr_in socket_addr;

	// this is bad, we want an xrt_frame_node etc.

	int way;

	struct xrt_frame_sink frame_sink;
	struct xrt_frame *xf;
	// this is the GL texture id used by the main renderer. This is what gst/ code should also be using.
	GLuint frame_texture_id;
	GLenum frame_texture_target;
	GLboolean frame_available;
};

#ifdef __cplusplus
extern "C" {
#endif

static const char *dotfilepath = "mrow";

/*!
 * @defgroup drv_vf Video Fileframeserver driver
 * @ingroup drv
 *
 * @brief Frameserver using a video file.
 */

/*!
 * Create a vf frameserver by opening a video file.
 *
 * @ingroup drv_vf
 */
struct xrt_fs *
vf_fs_open_file(struct xrt_frame_context *xfctx, const char *path);

/*!
 * Create a vf frameserver that uses the videotestsource.
 *
 * @ingroup drv_vf
 */
struct xrt_fs *
vf_fs_videotestsource(struct xrt_frame_context *xfctx, struct state_t *state);


#ifdef __cplusplus
}
#endif
