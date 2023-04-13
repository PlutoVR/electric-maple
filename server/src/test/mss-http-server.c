/*
 * This file is a part of MultiStreamServer
 *
 * (C) 2019, Collabora Ltd
 */

#include "mss-http-server.h"
#include "gresource-multistream-server.h"

#include <glib/gstdio.h>

#include <json-glib/json-glib.h>

#include <libsoup/soup-message.h>
#include <libsoup/soup-server.h>

struct _MssHttpServer
{
  GObject parent;

  SoupServer *soup_server;

  gchar *hls_dir;

  GSList *websocket_connections;
};

G_DEFINE_TYPE (MssHttpServer, mss_http_server, G_TYPE_OBJECT)

enum
{
  SIGNAL_WS_CLIENT_CONNECTED,
  SIGNAL_WS_CLIENT_DISCONNECTED,
  SIGNAL_SDP_ANSWER,
  SIGNAL_CANDIDATE,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

MssHttpServer *
mss_http_server_new ()
{
  return MSS_HTTP_SERVER (g_object_new (MSS_TYPE_HTTP_SERVER, NULL));
}

static void
http_cb (SoupServer *server, SoupMessage *msg, const char *path,
  GHashTable *query, SoupClientContext *client, gpointer user_data)
{
  GResource *res = multistream_server_get_resource();
  GBytes *bytes;

  if (g_str_equal (path, "/")) {
    path = "/index.html";
  }

  bytes = g_resource_lookup_data(res, path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  if (bytes) {
    SoupBuffer *buffer;
    gconstpointer data;
    gsize size;

    data = g_bytes_get_data (bytes, &size);

    buffer = soup_buffer_new_with_owner(data, size, bytes,
        (GDestroyNotify) g_bytes_unref);

    soup_message_body_append_buffer (msg->response_body, buffer);
    soup_buffer_free (buffer);

    if (g_str_has_suffix(path, ".js")) {
      soup_message_headers_append(msg->response_headers,
          "Content-Type", "text/javascript");
    } else if (g_str_has_suffix(path, ".css")) {
      soup_message_headers_append(msg->response_headers,
          "Content-Type", "text/css");
    } else if (g_str_has_suffix(path, ".svg")) {
      soup_message_headers_append(msg->response_headers,
          "Content-Type", "image/svg+xml");
    }

    soup_message_set_status (msg, SOUP_STATUS_OK);
  } else {
    soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
  }
}

static void
hls_cb (SoupServer *server, SoupMessage *msg, const char *path,
  GHashTable *query, SoupClientContext *client, gpointer user_data)
{
  gchar *file_name;
  gchar *file_path;
  gchar *buf;
  gsize buf_len;
  GError *error = NULL;
  MssHttpServer *self = MSS_HTTP_SERVER(user_data);

  file_name = g_path_get_basename (path);
  file_path = g_build_filename (self->hls_dir, file_name, NULL);

  if (g_file_get_contents (file_path, &buf, &buf_len, &error)) {
    SoupBuffer *buffer;

    buffer = soup_buffer_new_take ((guchar *) buf, buf_len);
    soup_message_body_append_buffer (msg->response_body, buffer);
    soup_buffer_free (buffer);

    if (g_str_has_suffix(path, ".m3u8")) {
      soup_message_headers_append(msg->response_headers,
          "Content-Type", "application/vnd.apple.mpegurl");
    }

    soup_message_set_status (msg, SOUP_STATUS_OK);
  } else {
    g_warning (error->message);
    g_clear_error (&error);
    soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
  }
}

static void
mss_http_server_handle_message (MssHttpServer *server,
  SoupWebsocketConnection *connection, GBytes *message)
{
  gsize length = 0;
  const gchar *msg_data = g_bytes_get_data (message, &length);
  JsonParser *parser = json_parser_new ();
  GError *error = NULL;

  if (json_parser_load_from_data (parser, msg_data, length, &error)) {
    JsonObject *msg = json_node_get_object (json_parser_get_root (parser));
    const gchar *msg_type;

    if (!json_object_has_member (msg, "msg")) {
      // Invalid message
      goto out;
    }

    msg_type = json_object_get_string_member (msg, "msg");
    if (g_str_equal(msg_type, "answer")) {
      const gchar *answer_sdp = json_object_get_string_member (msg, "sdp");
      g_debug ("Received answer:\n %s", answer_sdp);

      g_signal_emit (server, signals[SIGNAL_SDP_ANSWER], 0, connection,
          answer_sdp);
    } else if (g_str_equal(msg_type, "candidate")) {
      JsonObject *candidate;

      candidate = json_object_get_object_member (msg, "candidate");

      g_signal_emit (server, signals[SIGNAL_CANDIDATE], 0, connection,
          json_object_get_int_member (candidate, "sdpMLineIndex"),
          json_object_get_string_member (candidate, "candidate"));
    }
  } else {
    g_debug ("Error parsing message: %s", error->message);
    g_clear_error (&error);
  }

out:
  g_object_unref (parser);
}

static void
message_cb (SoupWebsocketConnection *connection, gint type, GBytes *message,
  gpointer user_data)
{
  mss_http_server_handle_message (MSS_HTTP_SERVER (user_data),
      connection, message);
}

static void
mss_http_server_remove_websocket_connection (MssHttpServer *server,
  SoupWebsocketConnection *connection)
{
  MssClientId client_id;

  client_id = g_object_get_data (G_OBJECT (connection), "client_id");

  server->websocket_connections = g_slist_remove (server->websocket_connections,
      client_id);

  g_signal_emit (server, signals [SIGNAL_WS_CLIENT_DISCONNECTED], 0, client_id);
}

static void
closed_cb (SoupWebsocketConnection *connection, gpointer user_data)
{
  g_debug ("Connection closed");

  mss_http_server_remove_websocket_connection (MSS_HTTP_SERVER (user_data),
      connection);
}

static void
mss_http_server_add_websocket_connection (MssHttpServer *server,
  SoupWebsocketConnection *connection)
{
  g_signal_connect (connection, "message", (GCallback) message_cb, server);
  g_signal_connect (connection, "closed", (GCallback) closed_cb, server);

  g_object_ref (connection);

  g_object_set_data (G_OBJECT (connection),
      "client_id", connection);

  server->websocket_connections =
      g_slist_append (server->websocket_connections, connection);

  g_signal_emit (server, signals [SIGNAL_WS_CLIENT_CONNECTED], 0, connection);
}

static void
websocket_cb (SoupServer *server, SoupWebsocketConnection *connection,
  const char *path, SoupClientContext *client, gpointer user_data)
{
  g_debug ("New connection from %s", soup_client_context_get_host (client));

  mss_http_server_add_websocket_connection (MSS_HTTP_SERVER (user_data),
      connection);
}

static void
mss_http_server_init (MssHttpServer *server)
{
  GError *error = NULL;

  server->soup_server = soup_server_new (NULL, NULL);
  server->hls_dir = g_dir_make_tmp ("mss-hls-XXXXXX", &error);
  g_assert_no_error (error);

  soup_server_add_handler (server->soup_server, NULL, http_cb, server, NULL);
  soup_server_add_handler (server->soup_server, "/hls/", hls_cb, server, NULL);
  soup_server_add_websocket_handler (server->soup_server, "/ws", NULL, NULL,
      websocket_cb, server, NULL);

  soup_server_listen_all (server->soup_server, 8080, 0, &error);
  g_assert_no_error (error);
}

const gchar *
mss_http_server_get_hls_dir (MssHttpServer *server)
{
  return server->hls_dir;
}

static void
mss_http_server_send_to_websocket_client (MssHttpServer *server,
  MssClientId client_id, JsonNode *msg)
{
  SoupWebsocketConnection *connection = client_id;
  SoupWebsocketState socket_state;

  if (!g_slist_find (server->websocket_connections, connection)) {
    g_warning ("Unknown websocket connection.");
    return;
  }

  socket_state = soup_websocket_connection_get_state (connection);

  if (socket_state == SOUP_WEBSOCKET_STATE_OPEN) {
    gchar *msg_str = json_to_string (msg, TRUE);

    soup_websocket_connection_send_text (connection, msg_str);

    g_free (msg_str);
  } else {
    g_warning ("Trying to send message using websocket that isn't open.");
  }
}

void
mss_http_server_send_sdp_offer (MssHttpServer *server, MssClientId client_id,
  const gchar *sdp)
{
  JsonBuilder *builder;
  JsonNode *root;

  g_debug ("Send offer: %s", sdp);

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "msg");
  json_builder_add_string_value(builder, "offer");

  json_builder_set_member_name (builder, "sdp");
  json_builder_add_string_value(builder, sdp);
  json_builder_end_object (builder);

  root = json_builder_get_root (builder);

  mss_http_server_send_to_websocket_client (server, client_id, root);

  json_node_unref (root);
  g_object_unref (builder);
}

void
mss_http_server_send_candidate (MssHttpServer *server, MssClientId client_id,
  guint mlineindex, const gchar *candidate)
{
  JsonBuilder *builder;
  JsonNode *root;

  g_debug ("Send candidate: %u %s", mlineindex, candidate);

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "msg");
  json_builder_add_string_value(builder, "candidate");

  json_builder_set_member_name (builder, "candidate");
  json_builder_begin_object(builder);
  json_builder_set_member_name (builder, "candidate");
  json_builder_add_string_value(builder, candidate);
  json_builder_set_member_name (builder, "sdpMLineIndex");
  json_builder_add_int_value(builder, mlineindex);
  json_builder_end_object(builder);
  json_builder_end_object(builder);

  root = json_builder_get_root (builder);

  mss_http_server_send_to_websocket_client (server, client_id, root);

  json_node_unref (root);
  g_object_unref (builder);
}

static void
mss_http_server_dispose (GObject *object)
{
  MssHttpServer *self = MSS_HTTP_SERVER (object);
  GDir *dir;

  soup_server_disconnect (self->soup_server);
  g_clear_object (&self->soup_server);


  dir = g_dir_open (self->hls_dir, 0, NULL);
  if (dir) {
    const gchar *filename;

    while ((filename = g_dir_read_name (dir))) {
      gchar *path = g_build_filename (self->hls_dir, filename, NULL);
      g_unlink (path);
      g_free (path);
    }
    g_dir_close (dir);
    g_rmdir (self->hls_dir);
  }

  g_clear_pointer (&self->hls_dir, g_free);
}

static void
mss_http_server_class_init (MssHttpServerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = mss_http_server_dispose;

  signals[SIGNAL_WS_CLIENT_CONNECTED] =
      g_signal_new ("ws-client-connected", G_OBJECT_CLASS_TYPE (klass),
          G_SIGNAL_RUN_LAST,
          0, NULL, NULL, NULL,
          G_TYPE_NONE, 1, G_TYPE_POINTER);

  signals[SIGNAL_WS_CLIENT_DISCONNECTED] =
      g_signal_new ("ws-client-disconnected", G_OBJECT_CLASS_TYPE (klass),
          G_SIGNAL_RUN_LAST,
          0, NULL, NULL, NULL,
          G_TYPE_NONE, 1, G_TYPE_POINTER);

  signals[SIGNAL_SDP_ANSWER] =
      g_signal_new ("sdp-answer", G_OBJECT_CLASS_TYPE (klass),
          G_SIGNAL_RUN_LAST,
          0, NULL, NULL, NULL,
          G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_STRING);

  signals[SIGNAL_CANDIDATE] =
      g_signal_new ("candidate", G_OBJECT_CLASS_TYPE (klass),
          G_SIGNAL_RUN_LAST,
          0, NULL, NULL, NULL,
          G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_STRING);
}
