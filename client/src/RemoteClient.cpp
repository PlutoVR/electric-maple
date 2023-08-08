// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Implementation for remote client
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#include "RemoteClient.h"

#include "GLSwapchain.h"
#include "gst/app_log.h"

#include "render.hpp"
#include "xr_platform_deps.h"
#include <GLES3/gl3.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <openxr/openxr.h>


#include "gst/em_connection.h"
#include "gst/em_stream_client.h"
#include <openxr/openxr_platform.h>
#include <vector>

struct _EmRemoteClient
{

	EmConnection *connection;
	EmStreamClient *stream_client;

	// std::unique_ptr<EglData> initialEglData;

	XrExtent2Di eye_extents;

	struct
	{
		XrInstance instance{XR_NULL_HANDLE};
		// XrSystemId system;
		XrSession session{XR_NULL_HANDLE};

	} xr_not_owned;

	struct
	{
		XrSpace worldSpace;
		XrSpace viewSpace;
		XrSwapchain swapchain;
	} xr_owned;

	GLSwapchain swapchainBuffers;
};

static void
em_remote_client_dispose(EmRemoteClient *rc)
{
	if (rc->stream_client) {
		em_stream_client_stop(rc->stream_client);
	}
	if (rc->connection) {
		em_connection_disconnect(rc->connection);
	}
	// stream client is not gobject (yet?)
	em_stream_client_destroy(&rc->stream_client);
	g_clear_object(&rc->connection);
	rc->swapchainBuffers.reset();
}
static void
em_remote_client_finalize(EmRemoteClient *rc)
{
	if (rc->xr_owned.swapchain != XR_NULL_HANDLE) {
		xrDestroySwapchain(rc->xr_owned.swapchain);
		rc->xr_owned.swapchain = XR_NULL_HANDLE;
	}

	if (rc->xr_owned.viewSpace != XR_NULL_HANDLE) {
		xrDestroySpace(rc->xr_owned.viewSpace);
		rc->xr_owned.viewSpace = XR_NULL_HANDLE;
	}

	if (rc->xr_owned.worldSpace != XR_NULL_HANDLE) {
		xrDestroySpace(rc->xr_owned.worldSpace);
		rc->xr_owned.worldSpace = XR_NULL_HANDLE;
	}
}

EmRemoteClient *
em_remote_client_new(EmConnection *connection,
                     EmStreamClient *stream_client,
                     XrInstance instance,
                     XrSession session,
                     const XrExtent2Di *eye_extents)
{
	EmRemoteClient *self = reinterpret_cast<EmRemoteClient *>(calloc(1, sizeof(EmRemoteClient)));
	self->connection = connection;
	self->stream_client = stream_client;
	self->eye_extents = *eye_extents;
	self->xr_not_owned.instance = instance;
	self->xr_not_owned.session = session;


	{
		ALOGI("FRED: Creating OpenXR Swapchain...");
		// OpenXR swapchain
		XrSwapchainCreateInfo swapchainInfo = {};
		swapchainInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
		swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainInfo.format = GL_SRGB8_ALPHA8;
		swapchainInfo.width = self->eye_extents.width * 2;
		swapchainInfo.height = self->eye_extents.height;
		swapchainInfo.sampleCount = 1;
		swapchainInfo.faceCount = 1;
		swapchainInfo.arraySize = 1;
		swapchainInfo.mipCount = 1;

		XrResult result =
		    xrCreateSwapchain(self->xr_not_owned.session, &swapchainInfo, &self->xr_owned.swapchain);

		if (XR_FAILED(result)) {
			ALOGE("Failed to create OpenXR swapchain (%d)\n", result);
			free(self);
			return nullptr;
		}
	}
	return self;
}

void
em_remote_client_destroy(EmRemoteClient **ptr_rc)
{

	if (ptr_rc == NULL) {
		return;
	}
	EmRemoteClient *rc = *ptr_rc;
	if (rc == NULL) {
		return;
	}
	em_remote_client_dispose(rc);
	em_remote_client_finalize(rc);
	free(rc);
	*ptr_rc = NULL;
}
