// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup xrt_fs_em
 */

#pragma once

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdbool.h>

struct em_fs;


struct em_sample
{
	GLuint frame_texture_id;
	GLenum frame_texture_target;
	// bool frame_available;
};
struct em_state
{
#ifdef __ANDROID__
	struct android_app *app;
	JNIEnv *jni;
	JavaVM *java_vm;
#endif
	bool hasPermissions;

	bool connected;

	// struct em_fs *vid;

	// EmConnection *connection;
	// EmStreamClient *stream_client;

	// This mutex protects the EGL context below across main and gstgl threads
	// struct os_mutex egl_lock;

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


	// this is bad, we want an xrt_frame_node etc.

	int way;

	// this is the GL texture id used by the main renderer.
	GLuint frame_texture_id;
	GLenum frame_texture_target;
	GLboolean frame_available;

	struct em_sample *prev_sample;
};
