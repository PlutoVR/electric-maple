#include <gst/gst.h>
#include <libsoup/soup-message.h>
#include <libsoup/soup-session.h>
#include <json-glib/json-glib.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#define U_LOG_E gst_print

typedef struct
{
	GMainLoop *loop;
	GstElement *pipeline;
	GstElement *webrtcbin;
	SoupWebsocketConnection *ws;
} Context;

void
send_sdp_answer(Context *self, const gchar *sdp)
{
	U_LOG_E("send_sdp_answer called!");
	JsonBuilder *builder;
	JsonNode *root;
	gchar *msg_str;

	U_LOG_E("Send answer: %s\n", sdp);

	builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "msg");
	json_builder_add_string_value(builder, "answer");

	json_builder_set_member_name(builder, "sdp");
	json_builder_add_string_value(builder, sdp);
	json_builder_end_object(builder);

	root = json_builder_get_root(builder);

	msg_str = json_to_string(root, TRUE);
	soup_websocket_connection_send_text(self->ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);
}

static void
webrtc_on_ice_candidate_cb(GstElement *webrtcbin, guint mlineindex, gchar *candidate, gpointer user_data)
{
	Context *self = user_data;
	U_LOG_E("webrtc_on_ice_candidate_cb called!");
	JsonBuilder *builder;
	JsonNode *root;
	gchar *msg_str;

	U_LOG_E("Send candidate: %u %s\n", mlineindex, candidate);

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
	soup_websocket_connection_send_text(self->ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);
}

static void
on_answer_created(GstPromise *promise, gpointer user_data)
{
	Context *self = user_data;
	U_LOG_E("on_answer_created called!");
	GstWebRTCSessionDescription *answer = NULL;
	gchar *sdp;

	gst_structure_get(gst_promise_get_reply(promise), "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
	gst_promise_unref(promise);

	if (NULL == answer) {
		U_LOG_E("on_answer_created : ERROR !  get_promise answer = null !");
	}

	g_signal_emit_by_name(self->webrtcbin, "set-local-description", answer, NULL);

	sdp = gst_sdp_message_as_text(answer->sdp);
	if (NULL == sdp) {
		U_LOG_E("on_answer_created : ERROR !  sdp = null !");
	}
	send_sdp_answer(self, sdp);
	g_free(sdp);

	gst_webrtc_session_description_free(answer);
}

static void
process_sdp_offer(Context *self, const gchar *sdp)
{
	U_LOG_E("process_sdp_offer called!");
	GstSDPMessage *sdp_msg = NULL;
	GstWebRTCSessionDescription *desc = NULL;


	U_LOG_E("Received offer: %s\n\n", sdp);

	if (gst_sdp_message_new_from_text(sdp, &sdp_msg) != GST_SDP_OK) {
		g_debug("Error parsing SDP description");
		goto out;
	}

	desc = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, sdp_msg);
	if (desc) {
		GstPromise *promise;

		promise = gst_promise_new();

		g_signal_emit_by_name(self->webrtcbin, "set-remote-description", desc, promise);

		gst_promise_wait(promise);
		gst_promise_unref(promise);

		g_signal_emit_by_name(
		    self->webrtcbin, "create-answer", NULL,
		    gst_promise_new_with_change_func((GstPromiseChangeFunc)on_answer_created, self, NULL));
	} else {
		gst_sdp_message_free(sdp_msg);
	}

out:
	g_clear_pointer(&desc, gst_webrtc_session_description_free);
}

static void
process_candidate(Context *self, guint mlineindex, const gchar *candidate)
{
	U_LOG_E("process_candidate called!");
	U_LOG_E("Received candidate: %d %s\n", mlineindex, candidate);

	g_signal_emit_by_name(self->webrtcbin, "add-ice-candidate", mlineindex, candidate);
}

static void
message_cb(SoupWebsocketConnection *connection, gint type, GBytes *message, gpointer user_data)
{
	Context *self = user_data;

	U_LOG_E("message_cb called!");
	gsize length = 0;
	const gchar *msg_data = g_bytes_get_data(message, &length);
	JsonParser *parser = json_parser_new();
	GError *error = NULL;

	if (json_parser_load_from_data(parser, msg_data, length, &error)) {
		JsonObject *msg = json_node_get_object(json_parser_get_root(parser));
		const gchar *msg_type;

		if (!json_object_has_member(msg, "msg")) {
			// Invalid message
			goto out;
		}

		msg_type = json_object_get_string_member(msg, "msg");
		U_LOG_E("Websocket message received: %s\n", msg_type);

		if (g_str_equal(msg_type, "offer")) {
			const gchar *offer_sdp = json_object_get_string_member(msg, "sdp");
			process_sdp_offer(self, offer_sdp);
		} else if (g_str_equal(msg_type, "candidate")) {
			JsonObject *candidate;

			candidate = json_object_get_object_member(msg, "candidate");

			process_candidate(self, json_object_get_int_member(candidate, "sdpMLineIndex"),
			                  json_object_get_string_member(candidate, "candidate"));
		}
	} else {
		g_debug("Error parsing message: %s", error->message);
		g_clear_error(&error);
	}

out:
	g_object_unref(parser);
}

static gboolean
gst_bus_cb(GstBus *bus, GstMessage *message, gpointer data)
{
	U_LOG_E("Received message: %" GST_PTR_FORMAT "\n", message);
	return G_SOURCE_CONTINUE;
}

static void
websocket_connected_cb(GObject *session, GAsyncResult *res, gpointer user_data)
{
	Context *self = user_data;
	GError *error = NULL;

	self->ws = soup_session_websocket_connect_finish(SOUP_SESSION(session), res, &error);
	g_assert_no_error(error);

	g_signal_connect(self->ws, "message", G_CALLBACK(message_cb), self);

	gchar *pipeline_string =
	    g_strdup_printf("webrtcbin name=webrtc bundle-policy=max-bundle ! decodebin ! glimagesink");
	GstElement *pipeline = gst_parse_launch(pipeline_string, &error);
	g_assert_no_error(error);

	self->webrtcbin = gst_bin_get_by_name(GST_BIN(pipeline), "webrtc");
	g_signal_connect(self->webrtcbin, "on-ice-candidate", G_CALLBACK(webrtc_on_ice_candidate_cb), self);

	g_autoptr(GstBus) bus = gst_element_get_bus(pipeline);
	gst_bus_add_watch(bus, gst_bus_cb, pipeline);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

int
main(int argc, char *argv[])
{
	Context ctx = {
	    NULL,
	};
	ctx.loop = g_main_loop_new(NULL, FALSE);

	SoupSession *soup_session = soup_session_new();
	soup_session_websocket_connect_async(soup_session, soup_message_new(SOUP_METHOD_GET, "ws://127.0.0.1:8080/ws"),
	                                     NULL, NULL, NULL, websocket_connected_cb, &ctx);

	g_main_loop_run(ctx.loop);
	g_object_unref(soup_session);
}
