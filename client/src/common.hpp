// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Common includes and state struct for WebRTC client.
 * @author Moshi Turner <moses@collabora.com>
 */

#pragma once

#include <cstring>
#include "util/u_logging.h"


#include <android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <jni.h>
#include <stdbool.h>


#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "pluto.pb.h"
#include "pb_encode.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

	// Remove this! This is a hack because Moshi didn't really have time to figure out how to get system time on
	// Android!
	XrTime last_predicted_display_time;


	int socket_fd;
	struct sockaddr_in socket_addr;
};
