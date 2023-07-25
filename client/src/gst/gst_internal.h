// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for GST part of ElectricMaple XR streaming frameserver
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @ingroup xrt_fs_em
 */
#pragma once

#include <gst/gstelement.h>
#include <libsoup/soup-session.h>

/// Type of a callback that will launch a pipeline and return a reference to it.
typedef GstElement *(*emh_launch_pipeline_callback)(gpointer user_data);


/*!
 * Data required for the handshake to complete.
 *
 * @implements xrt_frame_node
 * @implements xrt_fs
 */
struct em_handshake
{
	SoupSession *soup_session;
	SoupWebsocketConnection *ws;
	GstElement *webrtcbin;
	gchar *websocket_uri;
	struct
	{
		emh_launch_pipeline_callback callback;
		gpointer data;
	} launch_pipeline;
};

/*!
 * Prep the handshake struct fields
 *
 * @param emh self
 * @param callback Function pointer to call to create a pipeline
 * @param data Optional userdata to include when calling @p callback
 * @param websocket_uri The websocket URI to connect to. Ownership does not transfer (we copy it)
 *
 * @memberof em_handshake
 */
void
em_handshake_init(struct em_handshake *emh, emh_launch_pipeline_callback callback, gpointer data, gchar *websocket_uri);

/// Clean up the handshake struct fields
/// @memberof em_handshake
void
em_handshake_fini(struct em_handshake *emh);



void
em_gst_message_debug(const char *function, GstMessage *msg);

#define LOG_MSG(MSG)                                                                                                   \
	do {                                                                                                           \
		em_gst_message_debug(__FUNCTION__, MSG);                                                               \
	} while (0)
