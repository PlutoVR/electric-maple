// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Implementation for remote client
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#include "gst/app_log.h"

#include "xr_platform_deps.h"
#include <GLES3/gl3.h>
#include <cassert>
#include <cstdint>
#include <openxr/openxr.h>

#include "RemoteClient.h"

#include "gst/em_connection.h"
#include "gst/em_stream_client.h"
#include <openxr/openxr_platform.h>
#include <vector>

struct _EmRemoteClient
{

	EmConnection *connection;
	EmStreamClient *stream_client;

	EGLDisplay display;
	// context created in initializeEGL
	EGLContext context;
	// config used to create context
	EGLConfig config;
	// 16x16 pbuffer surface
	EGLSurface surface;

	struct
	{
		XrInstance instance;
		XrSystemId system;
		XrSession session;

	} xr_not_owned;

	struct
	{

	} xr_owned;
};
