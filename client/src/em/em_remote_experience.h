// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief  Header for full remote experience object
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#pragma once

#include "em_connection.h"
#include "em_stream_client.h"
#include <openxr/openxr.h>

typedef struct _EmRemoteExperience EmRemoteExperience;
typedef struct _EmConnection EmConnection;

typedef struct _em_proto_UpMessage em_proto_UpMessage;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum EmPollRenderResult
{
	EM_POLL_RENDER_RESULT_ERROR_EGL = -2,
	EM_POLL_RENDER_RESULT_ERROR_WAITFRAME = -1,
	EM_POLL_RENDER_RESULT_NO_SAMPLE_AVAILABLE = 0,
	EM_POLL_RENDER_RESULT_SHOULD_NOT_RENDER,
	EM_POLL_RENDER_RESULT_REUSED_SAMPLE,
	EM_POLL_RENDER_RESULT_NEW_SAMPLE,
} EmPollRenderResult;

static inline bool
em_poll_render_result_include_layer(EmPollRenderResult res)
{
	return res >= EM_POLL_RENDER_RESULT_REUSED_SAMPLE;
}

static inline bool
em_poll_render_result_is_error(EmPollRenderResult res)
{
	return res < 0;
}

static inline const char *
em_poll_render_result_to_string(EmPollRenderResult res)
{
#define MAKE_CASE(X)                                                                                                   \
	case X: return #X
	switch (res) {
		MAKE_CASE(EM_POLL_RENDER_RESULT_ERROR_EGL);
		MAKE_CASE(EM_POLL_RENDER_RESULT_ERROR_WAITFRAME);
		MAKE_CASE(EM_POLL_RENDER_RESULT_NO_SAMPLE_AVAILABLE);
		MAKE_CASE(EM_POLL_RENDER_RESULT_SHOULD_NOT_RENDER);
		MAKE_CASE(EM_POLL_RENDER_RESULT_REUSED_SAMPLE);
		MAKE_CASE(EM_POLL_RENDER_RESULT_NEW_SAMPLE);
	default: return "EM_POLL_RENDER_RESULT_unknown";
	}
}

/**
 * Create a remote experience object, which interacts with the stream client and OpenXR to provide a remotely-rendered
 * OpenXR experience.
 *
 * You must have enabled the XR_KHR_convert_timespec_time extension!
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
EmPollRenderResult
em_remote_experience_poll_and_render_frame(EmRemoteExperience *exp);

/*!
 * Check for a delivered frame, rendering it if available.
 *
 * @pre xrWaitFrame and xrBeginFrame have been called, as well as em_stream_client_egl_begin_pbuffer
 *
 * @param exp Self
 * @param beginFrameTime a timespec from CLOCK_MONOTONIC indicating when xrBeginFrame was called.
 * @param predictedDisplayTime the predicted display time from xrWaitFrame.
 * @param views an array of 2 XrView structures, populated.
 * @param projectionLayer a projection layer containing two views, partially populated. Will be populated further.
 * @param projectionViews an array of 2 projection view structures, initialized. Will be populated.
 */
EmPollRenderResult
em_remote_experience_inner_poll_and_render_frame(EmRemoteExperience *exp,
                                                 const struct timespec *beginFrameTime,
                                                 XrTime predictedDisplayTime,
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
em_remote_experience_emit_upmessage(EmRemoteExperience *exp, em_proto_UpMessage *upMessage);

#ifdef __cplusplus
} // extern "C"
#endif
