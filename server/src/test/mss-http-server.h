/*
 * This file is a part of MultiStreamServer
 *
 * (C) 2019, Collabora Ltd
 */

#ifndef __MSS_HTTP_SERVER_H__
#define __MSS_HTTP_SERVER_H__

#include <glib-object.h>

#define MSS_TYPE_HTTP_SERVER mss_http_server_get_type()

G_DECLARE_FINAL_TYPE (MssHttpServer, mss_http_server, MSS, HTTP_SERVER, GObject)

typedef gpointer MssClientId;

MssHttpServer *
mss_http_server_new ();

const gchar *
mss_http_server_get_hls_dir (MssHttpServer *server);

void
mss_http_server_send_sdp_offer (MssHttpServer *server, MssClientId client_id,
  const gchar *msg);

void
mss_http_server_send_candidate (MssHttpServer *server, MssClientId client_id,
  guint mlineindex, const gchar *candidate);

#endif /* __MSS_HTTP_SERVER_H__ */
