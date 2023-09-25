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

typedef struct _pluto_UpMessage pluto_UpMessage;

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
 *
 * Calls xrWaitFrame and xrBeginFrame, as well as xrEndFrame. See @ref em_remote_experience_inner_poll_and_render_frame
 * if you are doing this yourself in your app.
 */
void
em_remote_experience_poll_and_render_frame(EmRemoteExperience *exp);

/*!
 * Check for a delivered frame, rendering it if available.
 *
 * @pre xrWaitFrame and xrBeginFrame have been called, as well as em_stream_client_egl_begin_pbuffer
 *
 * @param exp Self
 * @param views an array of 2 XrView structures, populated.
 * @param projectionLayer a projection layer containing two views, partially populated. Will be populated further.
 * @param projectionViews an array of 2 projection view structures, initialized. Will be populated.
 */
void
em_remote_experience_inner_poll_and_render_frame(EmRemoteExperience *exp,
                                                 XrView *views,
                                                 XrCompositionLayerProjection *projectionLayer,
                                                 XrCompositionLayerProjectionView *projectionViews);

/*!
 * Set message ID, then serialize and send the upstream message given.
 *
 * Message ID is incremented atomically.
 *
 * @param exp Self
 * @param upMessage The upstream message, fully populated except for up_message_id.
 *
 * @return true on success
 */
bool
em_remote_experience_emit_upmessage(EmRemoteExperience *exp, pluto_UpMessage *upMessage);

#ifdef __cplusplus
} // extern "C"
#endif
