// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  WebRTC handshake for ElectricMaple XR streaming frameserver
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @ingroup xrt_fs_em
 */

#include "em_connection.h"

#include "em_status.h"
#include "app_log.h"


#include <gst/gstelement.h>
#include <gst/gstobject.h>
#include <stdbool.h>
#include <string.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>
#undef GST_USE_UNSTABLE_API

#include <libsoup/soup-message.h>
#include <libsoup/soup-session.h>

#include <json-glib/json-glib.h>

#define SECONDS_BETWEEN_RETRY (3)


#define MAKE_CASE(E)                                                                                                   \
	case E: return #E

static const char *
peer_connection_state_to_string(GstWebRTCPeerConnectionState state)
{

	switch (state) {
		MAKE_CASE(GST_WEBRTC_PEER_CONNECTION_STATE_NEW);
		MAKE_CASE(GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTING);
		MAKE_CASE(GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTED);
		MAKE_CASE(GST_WEBRTC_PEER_CONNECTION_STATE_DISCONNECTED);
		MAKE_CASE(GST_WEBRTC_PEER_CONNECTION_STATE_FAILED);
		MAKE_CASE(GST_WEBRTC_PEER_CONNECTION_STATE_CLOSED);
	default: return "!Unknown!";
	}
}
#undef MAKE_CASE

static void
emconn_update_status(struct em_connection *emconn, enum em_status status)
{
	if (status == emconn->status) {
		ALOGI("emconn: state update: already in %s", em_status_to_string(emconn->status));
		return;
	}
	ALOGI("emconn: state update: %s -> %s", em_status_to_string(emconn->status), em_status_to_string(status));
	emconn->status = status;
}

static void
emconn_update_status_from_peer_connection_state(struct em_connection *emconn, GstWebRTCPeerConnectionState state)
{
	switch (state) {
	case GST_WEBRTC_PEER_CONNECTION_STATE_NEW: break;
	case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTING: emconn_update_status(emconn, EM_STATUS_NEGOTIATING); break;
	case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTED:
		emconn_update_status(emconn, EM_STATUS_CONNECTED_NO_DATA);
		break;

	case GST_WEBRTC_PEER_CONNECTION_STATE_DISCONNECTED:
	case GST_WEBRTC_PEER_CONNECTION_STATE_CLOSED: emconn_update_status(emconn, EM_STATUS_IDLE_NOT_CONNECTED); break;

	case GST_WEBRTC_PEER_CONNECTION_STATE_FAILED: emconn_update_status(emconn, EM_STATUS_DISCONNECTED_ERROR); break;
	}
}

// static void
// emconn_update_status_from_webrtcbin(struct em_connection *emconn)
// {
// 	if (!emconn->webrtcbin) {
// 		return;
// 	}
// 	GstWebRTCPeerConnectionState state;
// 	g_object_get(emconn->webrtcbin, "connection-state", &state, NULL);
// 	ALOGI("RYLIE: webrtc peer connection state is %s", peer_connection_state_to_string(state));
// 	emconn_update_status_from_peer_connection_state(emconn, state);
// }

static void
emconn_disconnect_internal(struct em_connection *emconn, enum em_status status)
{

	if (emconn->timeout_src_id != 0) {
		g_source_remove(emconn->timeout_src_id);
		emconn->timeout_src_id = 0;
	}
	if (emconn->retry_timeout_src_id != 0) {
		g_source_remove(emconn->retry_timeout_src_id);
		emconn->retry_timeout_src_id = 0;
	}
	if (emconn->pipeline_launched) {
		(emconn->drop_pipeline.callback)(emconn->drop_pipeline.data);
		emconn->pipeline_launched = false;
	}
	if (emconn->ws) {
		g_object_unref(emconn->ws);
		emconn->ws = NULL;
	}
	gst_clear_object(&emconn->webrtcbin);
	gst_clear_object(&emconn->datachannel);
	emconn_update_status(emconn, status);
}


static void
emconn_data_channel_error_cb(GstWebRTCDataChannel *datachannel, gpointer user_data)
{
	struct em_connection *emconn = user_data;
	ALOGE("RYLIE: %s: error", __FUNCTION__);
	emconn_disconnect_internal(emconn, EM_STATUS_DISCONNECTED_ERROR);
	// abort();
}

static void
emconn_data_channel_close_cb(GstWebRTCDataChannel *datachannel, gpointer user_data)
{
	struct em_connection *emconn = user_data;
	ALOGE("RYLIE: %s: Data channel closed", __FUNCTION__);
	emconn_disconnect_internal(emconn, EM_STATUS_DISCONNECTED_REMOTE_CLOSE);
}

static void
emconn_data_channel_message_string_cb(GstWebRTCDataChannel *datachannel, gchar *str, gpointer user_data)
{
	ALOGE("RYLIE: %s: Received data channel message: %s", __FUNCTION__, str);
	struct em_connection *emconn = user_data;
}

// static gboolean
// emconn_data_channel_send_message(gpointer user_data)
// {
// 	ALOGI("RYLIE: Sending message to server");
// 	struct em_connection *emconn = user_data;
// 	g_signal_emit_by_name(emconn->datachannel, "send-string", "Hi! from Pluto client");

// 	return G_SOURCE_CONTINUE;
// }

static void
emconn_connect_internal(struct em_connection *emconn, enum em_status status);

static gboolean
emconn_retry_connect(gpointer user_data)
{
	struct em_connection *emconn = user_data;
	emconn_connect_internal(emconn, EM_STATUS_CONNECTING_RETRY);
	emconn->retry_timeout_src_id = 0;
	return G_SOURCE_REMOVE;
}

static void
emconn_webrtc_deep_notify_callback(GstObject *self, GstObject *prop_object, GParamSpec *prop, gpointer user_data)
{
	struct em_connection *emconn = user_data;
	GstWebRTCPeerConnectionState state;
	g_object_get(prop_object, "connection-state", &state, NULL);
	ALOGI("RYLIE: deep-notify callback says peer connection state is %s", peer_connection_state_to_string(state));
	emconn_update_status_from_peer_connection_state(emconn, state);
}

static void
emconn_webrtc_on_data_channel_cb(GstElement *webrtcbin, GstWebRTCDataChannel *data_channel, void *user_data)
{
	struct em_connection *emconn = user_data;

	// emconn_update_status_from_webrtcbin(emconn);
	ALOGE("Successfully created datachannel");

	g_assert_null(emconn->datachannel);

	emconn->datachannel = GST_WEBRTC_DATA_CHANNEL(data_channel);

	// emconn->timeout_src_id = g_timeout_add_seconds(3, emconn_data_channel_send_message, user_data);

	g_signal_connect(emconn->datachannel, "on-close", G_CALLBACK(emconn_data_channel_close_cb), user_data);
	g_signal_connect(emconn->datachannel, "on-error", G_CALLBACK(emconn_data_channel_error_cb), user_data);
	g_signal_connect(emconn->datachannel, "on-message-string", G_CALLBACK(emconn_data_channel_message_string_cb),
	                 user_data);
	emconn_update_status(emconn, EM_STATUS_CONNECTED);
}

static void
emconn_schedule_retry_connect(struct em_connection *emconn)
{
	ALOGI("Scheduling retry of connection in %d seconds", SECONDS_BETWEEN_RETRY);
	emconn->retry_timeout_src_id = g_timeout_add_seconds(SECONDS_BETWEEN_RETRY, emconn_retry_connect, emconn);
}


void
emconn_send_sdp_answer(struct em_connection *emconn, const gchar *sdp)
{
	ALOGI("[enter] %s", __FUNCTION__);
	JsonBuilder *builder;
	JsonNode *root;
	gchar *msg_str;

	ALOGE("Send answer: %s", sdp);

	builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "msg");
	json_builder_add_string_value(builder, "answer");

	json_builder_set_member_name(builder, "sdp");
	json_builder_add_string_value(builder, sdp);
	json_builder_end_object(builder);

	root = json_builder_get_root(builder);

	msg_str = json_to_string(root, TRUE);
	soup_websocket_connection_send_text(emconn->ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);

	// emconn_update_status(emconn, EM_STATUS_CONNECTED_NO_DATA);
	// emconn_update_status_from_webrtcbin(emconn);


	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emconn_webrtc_on_ice_candidate_cb(GstElement *webrtcbin, guint mlineindex, gchar *candidate, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);
	struct em_connection *emconn = user_data;
	JsonBuilder *builder;
	JsonNode *root;
	gchar *msg_str;

	ALOGE("Send candidate: line %u: %s", mlineindex, candidate);

	builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "msg");
	json_builder_add_string_value(builder, "candidate");

	json_builder_set_member_name(builder, "candidate");
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "candidate");
	json_builder_add_string_value(builder, candidate);
	json_builder_set_member_name(builder, "sdpMLineIndex");
	json_builder_add_int_value(builder, mlineindex);
	json_builder_end_object(builder);
	json_builder_end_object(builder);

	root = json_builder_get_root(builder);

	msg_str = json_to_string(root, TRUE);
	ALOGD("%s: candidate message: %s", __FUNCTION__, msg_str);
	soup_websocket_connection_send_text(emconn->ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);
	// emconn_update_status_from_webrtcbin(emconn);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_on_answer_created(GstPromise *promise, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);

	struct em_connection *emconn = user_data;
	GstWebRTCSessionDescription *answer = NULL;
	gchar *sdp;

	gst_structure_get(gst_promise_get_reply(promise), "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
	gst_promise_unref(promise);

	if (NULL == answer) {
		ALOGE("%s : ERROR !  get_promise answer = null !", __FUNCTION__);
	}

	g_signal_emit_by_name(emconn->webrtcbin, "set-local-description", answer, NULL);

	sdp = gst_sdp_message_as_text(answer->sdp);
	if (NULL == sdp) {
		ALOGE("%s : ERROR !  sdp = null !", __FUNCTION__);
	}
	emconn_send_sdp_answer(emconn, sdp);
	g_free(sdp);

	gst_webrtc_session_description_free(answer);
	// emconn_update_status_from_webrtcbin(emconn);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_process_sdp_offer(struct em_connection *emconn, const gchar *sdp)
{
	ALOGI("[enter] %s", __FUNCTION__);
	GstSDPMessage *sdp_msg = NULL;
	GstWebRTCSessionDescription *desc = NULL;


	ALOGE("Received offer: %s\n", sdp);

	if (gst_sdp_message_new_from_text(sdp, &sdp_msg) != GST_SDP_OK) {
		g_debug("Error parsing SDP description");
		goto out;
	}

	desc = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, sdp_msg);
	if (desc) {
		GstPromise *promise;

		promise = gst_promise_new();

		g_signal_emit_by_name(emconn->webrtcbin, "set-remote-description", desc, promise);

		gst_promise_wait(promise);
		gst_promise_unref(promise);

		g_signal_emit_by_name(
		    emconn->webrtcbin, "create-answer", NULL,
		    gst_promise_new_with_change_func((GstPromiseChangeFunc)emh_on_answer_created, emconn, NULL));
	} else {
		gst_sdp_message_free(sdp_msg);
	}
	// emconn_update_status_from_webrtcbin(emconn);

out:
	g_clear_pointer(&desc, gst_webrtc_session_description_free);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_process_candidate(struct em_connection *emconn, guint mlineindex, const gchar *candidate)
{
	ALOGI("process_candidate: %d %s", mlineindex, candidate);

	g_signal_emit_by_name(emconn->webrtcbin, "add-ice-candidate", mlineindex, candidate);
	// emconn_update_status_from_webrtcbin(emconn);
}

static void
emconn_on_ws_message_cb(SoupWebsocketConnection *connection, gint type, GBytes *message, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);
	struct em_connection *emconn = user_data;
	gsize length = 0;
	const gchar *msg_data = g_bytes_get_data(message, &length);
	JsonParser *parser = json_parser_new();
	GError *error = NULL;

	// TODO convert gsize to gssize after range check

	if (json_parser_load_from_data(parser, msg_data, length, &error)) {
		JsonObject *msg = json_node_get_object(json_parser_get_root(parser));
		const gchar *msg_type;

		if (!json_object_has_member(msg, "msg")) {
			// Invalid message
			goto out;
		}

		msg_type = json_object_get_string_member(msg, "msg");
		ALOGI("Websocket message received: %s", msg_type);

		if (g_str_equal(msg_type, "offer")) {
			const gchar *offer_sdp = json_object_get_string_member(msg, "sdp");
			emh_process_sdp_offer(emconn, offer_sdp);
		} else if (g_str_equal(msg_type, "candidate")) {
			JsonObject *candidate;

			candidate = json_object_get_object_member(msg, "candidate");

			emh_process_candidate(emconn, json_object_get_int_member(candidate, "sdpMLineIndex"),
			                      json_object_get_string_member(candidate, "candidate"));
		}
	} else {
		g_debug("Error parsing message: %s", error->message);
		g_clear_error(&error);
	}

out:
	g_object_unref(parser);
	ALOGI("[exit]  %s", __FUNCTION__);
}


static void
emconn_websocket_connected_cb(GObject *session, GAsyncResult *res, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);
	struct em_connection *emconn = user_data;

	GError *error = NULL;

	g_assert(!emconn->ws);

	emconn->ws = soup_session_websocket_connect_finish(SOUP_SESSION(session), res, &error);
	// TODO if we couldn't connect, we actually have an error here, handle this better
	if (error) {
		ALOGW("Websocket connection failed, may not be available. Will retry.");
		emconn_update_status(emconn, EM_STATUS_WILL_RETRY);
	}
	g_assert_no_error(error);
	GstBus *bus;

	ALOGI("RYLIE: Websocket connected");
	g_signal_connect(emconn->ws, "message", G_CALLBACK(emconn_on_ws_message_cb), emconn);

	ALOGI("RYLIE: launching pipeline");
	g_autoptr(GstElement) pipeline =
	    gst_object_ref_sink(emconn->launch_pipeline.callback(emconn->launch_pipeline.data));

	emconn->pipeline_launched = true;
	emconn_update_status(emconn, EM_STATUS_NEGOTIATING);

	ALOGI("RYLIE: getting webrtcbin");
	emconn->webrtcbin = gst_bin_get_by_name(GST_BIN(pipeline), "webrtc");
	g_assert_nonnull(emconn->webrtcbin);
	g_assert(G_IS_OBJECT(emconn->webrtcbin));
	g_signal_connect(emconn->webrtcbin, "on-ice-candidate", G_CALLBACK(emconn_webrtc_on_ice_candidate_cb), emconn);
	g_signal_connect(emconn->webrtcbin, "on-data-channel", G_CALLBACK(emconn_webrtc_on_data_channel_cb), emconn);
	g_signal_connect(emconn->webrtcbin, "deep-notify::connection-state",
	                 G_CALLBACK(emconn_webrtc_deep_notify_callback), emconn);
	ALOGI("[exit]  %s", __FUNCTION__);
}
static void
emconn_connect_internal(struct em_connection *emconn, enum em_status status)
{
	em_connection_disconnect(emconn);

	ALOGE("RYLIE: calling soup_session_websocket_connect_async. websocket_uri = %s", emconn->websocket_uri);
#if SOUP_MAJOR_VERSION == 2
	soup_session_websocket_connect_async(emconn->soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, emconn->websocket_uri), // message
	                                     NULL,                                                     // origin
	                                     NULL,                                                     // protocols
	                                     NULL,                                                     // cancellable
	                                     emconn_websocket_connected_cb,                            // callback
	                                     emconn);                                                  // user_data

#else
	soup_session_websocket_connect_async(emconn->soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, emconn->websocket_uri), // message
	                                     NULL,                                                     // origin
	                                     NULL,                                                     // protocols
	                                     0,                                                        // io_prority
	                                     NULL,                                                     // cancellable
	                                     emh_websocket_connected_cb,                               // callback
	                                     emconn);                                                  // user_data

#endif
	emconn_update_status(emconn, status);
}

void
em_connection_connect(struct em_connection *emconn)
{
	ALOGI("[enter] %s", __FUNCTION__);
	emconn_connect_internal(emconn, EM_STATUS_CONNECTING);
	ALOGI("[exit]  %s", __FUNCTION__);
}

void
em_connection_disconnect(struct em_connection *emconn)
{
	emconn_disconnect_internal(emconn, EM_STATUS_IDLE_NOT_CONNECTED);
}

bool
em_connection_send_bytes(struct em_connection *emconn, GBytes *bytes)
{
	if (emconn->status != EM_STATUS_CONNECTED) {
		ALOGW("RYLIE: Cannot send bytes when status is %s", em_status_to_string(emconn->status));
		return false;
	}

	gboolean success = gst_webrtc_data_channel_send_data_full(emconn->datachannel, bytes, NULL);

	return success == TRUE;
}

void
em_connection_init(struct em_connection *emconn,
                   emconn_launch_pipeline_callback launch_callback,
                   emconn_drop_pipeline_callback drop_callback,
                   gpointer data,
                   gchar *websocket_uri)
{
	ALOGI("[enter] %s", __FUNCTION__);
	memset(emconn, 0, sizeof(*emconn));
	// emconn->soup_session = NULL;
	// emconn->ws = NULL;
	// emconn->webrtcbin = NULL;
	emconn->websocket_uri = g_strdup(websocket_uri);
	// emconn->datachannel = NULL;
	// emconn->timeout_src_id = 0;
	emconn->launch_pipeline.callback = launch_callback;
	emconn->launch_pipeline.data = data;
	emconn->drop_pipeline.callback = drop_callback;
	emconn->drop_pipeline.data = data;
	emconn->soup_session = soup_session_new();
	// emconn_connect(emconn);
	ALOGI("[exit]  %s", __FUNCTION__);
}

void
em_connection_fini(struct em_connection *emconn)
{
	ALOGI("[enter] %s", __FUNCTION__);

	em_connection_disconnect(emconn);

	g_free(emconn->websocket_uri);


	if (emconn->soup_session) {
		g_object_unref(emconn->soup_session);
		emconn->soup_session = NULL;
	}
	ALOGI("[exit]  %s", __FUNCTION__);
}
