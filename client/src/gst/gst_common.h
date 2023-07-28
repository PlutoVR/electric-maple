// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup xrt_fs_em
 */

#pragma once

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES

#include "xrt/xrt_frame.h"
#include "os/os_threading.h"

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <jni.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdbool.h>

#include "xrt/xrt_frameserver.h"

struct em_fs;

struct em_sample
{
	GLuint frame_texture_id;
	GLenum frame_texture_target;
	// bool frame_available;
};
struct em_state
{
	struct android_app *app;
	JNIEnv *jni;
	JavaVM *java_vm;
	bool hasPermissions;

	struct em_fs *vid;

	// This mutex protects the EGL context below across main and gstgl threads
	struct os_mutex egl_lock;

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

	// TODO This socket is for sending the HMD pose upstream "out of band" - replace with data channel.
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

	struct em_sample *prev_sample;
};


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Create an ElectricMaple XR streaming frameserver with default parameters.
 *
 * Must call from a thread in which we can safely make @p context active.
 */
struct xrt_fs *
em_fs_create_streaming_client(struct xrt_frame_context *xfctx,
                              struct em_state *state,
                              EGLDisplay display,
                              EGLContext context);

/*!
 * Attempt to retrieve a sample.
 *
 * @pre The Android main EGL context must be active with the lock held when calling.
 *
 * @return null if no new frame is received.
 */
struct em_sample *
em_fs_try_pull_sample(struct xrt_fs *fs);

/*!
 * Release a sample when no longer used.
 */
void
em_fs_release_sample(struct xrt_fs *fs, struct em_sample *ems);



#ifdef __cplusplus
}
#endif
