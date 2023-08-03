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
#include "em_status.h"

#include <gst/gstelement.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc_fwd.h>
#undef GST_USE_UNSTABLE_API

#include <libsoup/soup-session.h>

#include <stdbool.h>

/// Type of a callback that will launch a pipeline and return a reference to it.
typedef GstElement *(*emconn_launch_pipeline_callback)(gpointer user_data);

/// Type of a callback on connection drop/shutdown.
typedef void (*emconn_drop_pipeline_callback)(gpointer user_data);

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
	guint retry_timeout_src_id;

	enum em_status status;
	/// has the pipeline been launched (@ref launch_pipeline called)
	bool pipeline_launched;

	struct
	{
		emconn_launch_pipeline_callback callback;
		gpointer data;
	} launch_pipeline;

	struct
	{
		emconn_drop_pipeline_callback callback;
		gpointer data;
	} drop_pipeline;
};

/*!
 * Prep the handshake struct fields
 *
 * @param emconn self
 * @param launch_callback Function pointer to call to create a pipeline
 * @param drop_callback Function pointer to call when dropping a connection - must be idempotent
 * @param data Optional userdata to include when calling @p launch_callback and @p drop_callback
 * @param websocket_uri The websocket URI to connect to. Ownership does not transfer (we copy it)
 *
 * @memberof em_handshake
 */
void
em_connection_init(struct em_connection *emconn,
                   emconn_launch_pipeline_callback launch_callback,
                   emconn_drop_pipeline_callback drop_callback,
                   gpointer data,
                   gchar *websocket_uri);

/*!
 * Actually start connecting to the server
 */
void
em_connection_connect(struct em_connection *emconn);


/*!
 * Drop the server connection, if any.
 */
void
em_connection_disconnect(struct em_connection *emconn);


/*!
 * Send a message to the server
 */
bool
em_connection_send_bytes(struct em_connection *emconn, GBytes *bytes);

/// Clean up the handshake struct fields
/// @memberof em_handshake
void
em_connection_fini(struct em_connection *emconn);
