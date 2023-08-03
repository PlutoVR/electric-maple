// Copyright 2022-2023, PlutoVR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for the connection module of the ElectricMaple XR streaming solution
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */
#pragma once


#include <glib-object.h>

#include <stdbool.h>

#define EM_TYPE_CONNECTION em_connection_get_type()

G_DECLARE_FINAL_TYPE(EmConnection, em_connection, EM, CONNECTION, GObject)

/*
 * @param launch_callback Function pointer to call to create a pipeline
 * @param drop_callback Function pointer to call when dropping a connection - must be idempotent
 * @param data Optional userdata to include when calling @p launch_callback and @p drop_callback
 */

/*!
 * Create a connection object
 *
 * @param websocket_uri The websocket URI to connect to. Ownership does not transfer (we copy it)
 *
 * @memberof EmConnection
 */
EmConnection *
em_connection_new(gchar *websocket_uri);


/*!
 * Actually start connecting to the server
 *
 * @memberof EmConnection
 */
void
em_connection_connect(EmConnection *emconn);


/*!
 * Drop the server connection, if any.
 *
 * @memberof EmConnection
 */
void
em_connection_disconnect(EmConnection *emconn);


/*!
 * Send a message to the server
 *
 * @memberof EmConnection
 */
bool
em_connection_send_bytes(EmConnection *emconn, GBytes *bytes);

// /// Clean up the handshake struct fields
// /// @memberof em_handshake
// void
// em_connection_fini(struct em_connection *emconn);
