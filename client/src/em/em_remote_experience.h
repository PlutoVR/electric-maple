// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Header for full remote experience object
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#pragma once

#include "em_connection.h"
#include "em_stream_client.h"
#include <openxr/openxr.h>

typedef struct _EmRemoteExperience EmRemoteExperience;
typedef struct _EmConnection EmConnection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a remote experience object, which interacts with the stream client and OpenXR to provide a remotely-rendered
 * OpenXR experience..
 *
 * @param connection Your connection: we sink a ref. Used to send reports upstream.
 * @param stream_client Your stream client: we take ownership.
 * @param instance Your OpenXR instance: we only observe, do not take ownership.
 * @param session Your OpenXR session: we only observe, do not take ownership.
 * @param eye_extents Dimensions of the eye swapchain (max)
 *
 * @return EmRemoteExperience* or NULL in case of error
 */
EmRemoteExperience *
em_remote_experience_new(EmConnection *connection,
                         EmStreamClient *stream_client,
                         XrInstance instance,
                         XrSession session,
                         const XrExtent2Di *eye_extents);


/*!
 * Clear a pointer and free the associate remote experience object, if any.
 *
 * Handles null checking for you.
 */
void
em_remote_experience_destroy(EmRemoteExperience **ptr_exp);

/*!
 * Check for a delivered frame, rendering it if available.
 */
void
em_remote_experience_poll_and_render_frame(EmRemoteExperience *exp);

#ifdef __cplusplus
} // extern "C"
#endif
