// Copyright 2019-2023, Collabora, Ltd.
// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A GStreamer pipeline for WebRTC streaming
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakub Adam <jakub.adam@collabora.com>
 * @author Nicolas Dufresne <nicolas.dufresne@collabora.com>
 * @author Olivier Crête <olivier.crete@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include <glib-object.h>

#define MSS_TYPE_HTTP_SERVER mss_http_server_get_type()

G_DECLARE_FINAL_TYPE(MssHttpServer, mss_http_server, MSS, HTTP_SERVER, GObject)

typedef gpointer MssClientId;

MssHttpServer *
mss_http_server_new();

void
mss_http_server_send_sdp_offer(MssHttpServer *server, MssClientId client_id, const gchar *msg);

void
mss_http_server_send_candidate(MssHttpServer *server, MssClientId client_id, guint mlineindex, const gchar *candidate);
