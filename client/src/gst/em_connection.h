// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for the connection module of the ElectricMaple XR streaming solution
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */
#pragma once


#include <glib-object.h>
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


#define EM_TYPE_CONNECTION em_connection_get_type()

G_DECLARE_FINAL_TYPE(EmConnection, em_connection, EM, CONNECTION, GObject)

/*!
 * Create a connection object
 *
 * @param launch_callback Function pointer to call to create a pipeline
 * @param drop_callback Function pointer to call when dropping a connection - must be idempotent
 * @param data Optional userdata to include when calling @p launch_callback and @p drop_callback
 * @param websocket_uri The websocket URI to connect to. Ownership does not transfer (we copy it)
 *
 * @memberof EmConnection
 */
EmConnection *
em_connection_new(emconn_launch_pipeline_callback launch_callback,
                  emconn_drop_pipeline_callback drop_callback,
                  gpointer data,
                  gchar *websocket_uri);


/*!
 * Actually start connecting to the server
 */
void
em_connection_connect(EmConnection *emconn);


/*!
 * Drop the server connection, if any.
 */
void
em_connection_disconnect(EmConnection *emconn);


/*!
 * Send a message to the server
 */
bool
em_connection_send_bytes(EmConnection *emconn, GBytes *bytes);

// /// Clean up the handshake struct fields
// /// @memberof em_handshake
// void
// em_connection_fini(struct em_connection *emconn);
