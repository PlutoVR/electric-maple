// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Header for full remote client
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#pragma once

#include "gst/em_connection.h"
#include "gst/em_stream_client.h"
#include <openxr/openxr.h>

typedef struct _EmRemoteClient EmRemoteClient;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a remote client.
 *
 * @param connection Your connection: we sink a ref.
 * @param stream_client Your stream client: we sink a ref.
 * @param instance Your OpenXR instance: we only observe, do not take ownership.
 * @param session Your OpenXR session: we only observe, do not take ownership.
 * @param eye_extents Dimensions of the eye swapchain (max)
 * @return EmRemoteClient*
 */
EmRemoteClient *
em_remote_client_new(EmConnection *connection,
                     EmStreamClient *stream_client,
                     XrInstance instance,
                     XrSession session,
                     const XrExtent2Di *eye_extents);


/*!
 * Clear a pointer and free the associate remote client, if any.
 *
 * Handles null checking for you.
 */
void
em_remote_client_destroy(EmRemoteClient **ptr_rc);

#ifdef __cplusplus
} // extern "C"
#endif
