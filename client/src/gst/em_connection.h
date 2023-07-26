// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for the connection module of the ElectricMaple XR streaming solution
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @ingroup xrt_fs_em
 */
#pragma once

#include <gst/gstelement.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc_fwd.h>
#undef GST_USE_UNSTABLE_API

#include <libsoup/soup-session.h>

/// Type of a callback that will launch a pipeline and return a reference to it.
typedef GstElement *(*emconn_launch_pipeline_callback)(gpointer user_data);


/*!
 * Data required for the handshake to complete and to maintain the connection.
 */
struct em_connection
{
	SoupSession *soup_session;
	SoupWebsocketConnection *ws;
	GstElement *webrtcbin;
	gchar *websocket_uri;
	GstWebRTCDataChannel *datachannel;
	guint timeout_src_id;
	struct
	{
		emconn_launch_pipeline_callback callback;
		gpointer data;
	} launch_pipeline;
};

/*!
 * Prep the handshake struct fields
 *
 * @param emconn self
 * @param callback Function pointer to call to create a pipeline
 * @param data Optional userdata to include when calling @p callback
 * @param websocket_uri The websocket URI to connect to. Ownership does not transfer (we copy it)
 *
 * @memberof em_handshake
 */
void
em_connection_init(struct em_connection *emconn,
                   emconn_launch_pipeline_callback callback,
                   gpointer data,
                   gchar *websocket_uri);

/*!
 * Actually start connecting to the server
 */
void
em_connection_connect(struct em_connection *emconn);

/// Clean up the handshake struct fields
/// @memberof em_handshake
void
em_connection_fini(struct em_connection *emconn);
