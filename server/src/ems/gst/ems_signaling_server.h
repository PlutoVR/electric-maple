// Copyright 2019-2023, Collabora, Ltd.
// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  WebSocket signaling server for Electric Maple
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakub Adam <jakub.adam@collabora.com>
 * @author Nicolas Dufresne <nicolas.dufresne@collabora.com>
 * @author Olivier Crête <olivier.crete@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include <glib-object.h>

#define EMS_TYPE_SIGNALING_SERVER ems_signaling_server_get_type()

G_DECLARE_FINAL_TYPE(EmsSignalingServer, ems_signaling_server, EMS, SIGNALING_SERVER, GObject)

typedef gpointer EmsClientId;

EmsSignalingServer *
ems_signaling_server_new();

void
ems_signaling_server_send_sdp_offer(EmsSignalingServer *server, EmsClientId client_id, const gchar *msg);

void
ems_signaling_server_send_candidate(EmsSignalingServer *server,
                                    EmsClientId client_id,
                                    guint mlineindex,
                                    const gchar *candidate);
