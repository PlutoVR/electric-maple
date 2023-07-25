// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  WebRTC handshake for ElectricMaple XR streaming frameserver
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @ingroup xrt_fs_em
 */

#include "gst/gstelement.h"
#include "gst/gstobject.h"
#include "gst_internal.h"
#include "app_log.h"


#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include <libsoup/soup-message.h>
#include <libsoup/soup-session.h>

#include <json-glib/json-glib.h>

void
emh_send_sdp_answer(struct em_handshake *emh, const gchar *sdp)
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
	soup_websocket_connection_send_text(emh->ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_webrtc_on_ice_candidate_cb(GstElement *webrtcbin, guint mlineindex, gchar *candidate, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);
	struct em_handshake *emh = user_data;
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
	soup_websocket_connection_send_text(emh->ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_on_answer_created(GstPromise *promise, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);

	struct em_handshake *emh = user_data;
	GstWebRTCSessionDescription *answer = NULL;
	gchar *sdp;

	gst_structure_get(gst_promise_get_reply(promise), "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
	gst_promise_unref(promise);

	if (NULL == answer) {
		ALOGE("%s : ERROR !  get_promise answer = null !", __FUNCTION__);
	}

	g_signal_emit_by_name(emh->webrtcbin, "set-local-description", answer, NULL);

	sdp = gst_sdp_message_as_text(answer->sdp);
	if (NULL == sdp) {
		ALOGE("%s : ERROR !  sdp = null !", __FUNCTION__);
	}
	emh_send_sdp_answer(emh, sdp);
	g_free(sdp);

	gst_webrtc_session_description_free(answer);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_process_sdp_offer(struct em_handshake *emh, const gchar *sdp)
{
	ALOGI("[enter] %s", __FUNCTION__);
	GstSDPMessage *sdp_msg = NULL;
	GstWebRTCSessionDescription *desc = NULL;


	ALOGE("Received offer: %s\n\n", sdp);

	if (gst_sdp_message_new_from_text(sdp, &sdp_msg) != GST_SDP_OK) {
		g_debug("Error parsing SDP description");
		goto out;
	}

	desc = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, sdp_msg);
	if (desc) {
		GstPromise *promise;

		promise = gst_promise_new();

		g_signal_emit_by_name(emh->webrtcbin, "set-remote-description", desc, promise);

		gst_promise_wait(promise);
		gst_promise_unref(promise);

		g_signal_emit_by_name(
		    emh->webrtcbin, "create-answer", NULL,
		    gst_promise_new_with_change_func((GstPromiseChangeFunc)emh_on_answer_created, emh, NULL));
	} else {
		gst_sdp_message_free(sdp_msg);
	}

out:
	g_clear_pointer(&desc, gst_webrtc_session_description_free);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_process_candidate(struct em_handshake *emh, guint mlineindex, const gchar *candidate)
{
	ALOGI("process_candidate: %d %s\n", mlineindex, candidate);

	g_signal_emit_by_name(emh->webrtcbin, "add-ice-candidate", mlineindex, candidate);
}

static void
emh_on_ws_message_cb(SoupWebsocketConnection *connection, gint type, GBytes *message, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);
	struct em_handshake *emh = user_data;
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
		ALOGI("Websocket message received: %s\n", msg_type);

		if (g_str_equal(msg_type, "offer")) {
			const gchar *offer_sdp = json_object_get_string_member(msg, "sdp");
			emh_process_sdp_offer(emh, offer_sdp);
		} else if (g_str_equal(msg_type, "candidate")) {
			JsonObject *candidate;

			candidate = json_object_get_object_member(msg, "candidate");

			emh_process_candidate(emh, json_object_get_int_member(candidate, "sdpMLineIndex"),
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
emh_websocket_connected_cb(GObject *session, GAsyncResult *res, gpointer user_data)
{
	ALOGI("[enter] %s", __FUNCTION__);
	struct em_handshake *emh = user_data;

	GError *error = NULL;

	g_assert(!emh->ws);
	struct em_fs *vid = (struct em_fs *)user_data;

	emh->ws = soup_session_websocket_connect_finish(SOUP_SESSION(session), res, &error);
	g_assert_no_error(error);
	GstBus *bus;

	ALOGI("RYLIE: Websocket connected");
	g_signal_connect(emh->ws, "message", G_CALLBACK(emh_on_ws_message_cb), emh);

	ALOGI("RYLIE: launching pipeline");
	g_autoptr(GstElement) pipeline = gst_object_ref_sink(emh->launch_pipeline.callback(emh->launch_pipeline.data));


	ALOGI("RYLIE: getting webrtcbin");
	emh->webrtcbin = gst_bin_get_by_name(GST_BIN(pipeline), "webrtc");
	g_assert_nonnull(emh->webrtcbin);
	g_assert(G_IS_OBJECT(emh->webrtcbin));
	g_signal_connect(emh->webrtcbin, "on-ice-candidate", G_CALLBACK(emh_webrtc_on_ice_candidate_cb), emh);
	ALOGI("[exit]  %s", __FUNCTION__);
}

static void
emh_connect(struct em_handshake *emh)
{
	ALOGI("[enter] %s", __FUNCTION__);
	if (emh->ws) {
		g_object_unref(emh->ws);
		emh->ws = NULL;
	}
	ALOGE("RYLIE: calling soup_session_websocket_connect_async. websocket_uri = %s", emh->websocket_uri);
#if SOUP_MAJOR_VERSION == 2
	soup_session_websocket_connect_async(emh->soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, emh->websocket_uri), // message
	                                     NULL,                                                  // origin
	                                     NULL,                                                  // protocols
	                                     NULL,                                                  // cancellable
	                                     emh_websocket_connected_cb,                            // callback
	                                     emh);                                                  // user_data

#else
	soup_session_websocket_connect_async(emh->soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, emh->websocket_uri), // message
	                                     NULL,                                                  // origin
	                                     NULL,                                                  // protocols
	                                     0,                                                     // io_prority
	                                     NULL,                                                  // cancellable
	                                     emh_websocket_connected_cb,                            // callback
	                                     emh);                                                  // user_data

#endif
	ALOGI("[exit]  %s", __FUNCTION__);
}

void
em_handshake_init(struct em_handshake *emh, emh_launch_pipeline_callback callback, gpointer data, gchar *websocket_uri)
{
	ALOGI("[enter] %s", __FUNCTION__);
	emh->soup_session = NULL;
	emh->webrtcbin = NULL;
	emh->ws = NULL;
	emh->websocket_uri = g_strdup(websocket_uri);
	emh->launch_pipeline.callback = callback;
	emh->launch_pipeline.data = data;
	emh->soup_session = soup_session_new();
	emh_connect(emh);
	ALOGI("[exit]  %s", __FUNCTION__);
}

void
em_handshake_fini(struct em_handshake *emh)
{
	ALOGI("[enter] %s", __FUNCTION__);

	gst_clear_object(&emh->webrtcbin);
	g_free(emh->websocket_uri);
	if (emh->ws) {
		g_object_unref(emh->ws);
		emh->ws = NULL;
	}
	if (emh->soup_session) {
		g_object_unref(emh->soup_session);
		emh->soup_session = NULL;
	}
	ALOGI("[exit]  %s", __FUNCTION__);
}
