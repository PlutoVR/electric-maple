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

#include "os/os_time.h"
#include "os/os_threading.h"

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

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/app/gstappsink.h>
#include <gst/gstelement.h>
#include <gst/video/video-frame.h>

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
// FIXME: set relative path through include-dir
#include "/home/fredinfinite23/code/PlutoVR/linux-streaming-CLIENT2/monado/src/xrt/auxiliary/gstreamer/gstjniutils.h"

struct state_t
{
	struct android_app *app;
	JNIEnv *jni;
	JavaVM *java_vm;
	bool hasPermissions;

	struct vf_fs *vid;

	// This mutex protects the EGL context below across main and gstgl threads
	struct os_mutex egl_lock;

	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
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

// FIXME : MERGE VID AND STATE !!!! THIS IS SO UGLY !!
struct vf_fs
{
	struct xrt_fs base;

	struct os_thread_helper play_thread;

	GMainLoop *loop;
	GstElement *pipeline;
	GstGLDisplay *gst_gl_display;
	GstGLContext *gst_gl_context;
	GstGLContext *gst_gl_other_context;

	GstGLDisplay *display;
	GstGLContext *other_context;
	GstGLContext *context;
	GstElement *appsink;
	// GLenum texture_target; // WE SHOULD USE RENDER'S texture target
	// GLuint texture_id; // WE SHOULD USE RENDER'S texture id

	int width;
	int height;
	enum xrt_format format;
	enum xrt_stereo_format stereo_format;

	struct xrt_frame_node node;

	struct
	{
		bool extended_format;
		bool timeperframe;
	} has;

	enum xrt_fs_capture_type capture_type;
	struct xrt_frame_sink *sink;

	uint32_t selected;

	struct xrt_fs_capture_parameters capture_params;

	bool is_configured;
	bool is_running;
	enum u_logging_level log_level;

	struct state_t *state;
	JavaVM *java_vm;
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
vf_fs_gst_pipeline(struct xrt_frame_context *xfctx, struct state_t *state);


#ifdef __cplusplus
}
#endif
