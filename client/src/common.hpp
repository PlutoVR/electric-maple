// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Common includes and state struct for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 */

#pragma once

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES

#include <android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <jni.h>

// FIXME: THE BELOW ARE UGLY !?
#include "../../../../../../../.gradle/caches/transforms-3/0ea571ec8b0b3b9231cb793af03e2479/transformed/jetified-openxr_loader_for_android-1.0.20/prefab/modules/openxr_loader/include/openxr/openxr.h"
#include "../../../../../../../.gradle/caches/transforms-3/0ea571ec8b0b3b9231cb793af03e2479/transformed/jetified-openxr_loader_for_android-1.0.20/prefab/modules/openxr_loader/include/openxr/openxr_platform.h"

#include "../../../../proto/generated/pluto.pb.h"
#include "../../../../monado/src/external/nanopb/pb_encode.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <c++/v1/string.h>
#include <c++/v1/stdbool.h>

#define XR_LOAD(fn) xrGetInstanceProcAddr(state.instance, #fn, (PFN_xrVoidFunction *)&fn);

struct state_t
{
	struct android_app *app;
	JNIEnv *jni;
	bool hasPermissions;
	ANativeWindow *window;
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
	int frame_idx;

	struct xrt_frame_sink frame_sink;
	xrt_frame *xf = NULL;
	GLuint frame_tex;
};



// render.cpp

void
initializeEGL(struct state_t &state);

void
setupRender();

void
draw(GLuint framebuffer, GLuint texture);



//
