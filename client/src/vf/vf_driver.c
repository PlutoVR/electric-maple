// Copyright 2020-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Video file frameserver implementation
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup drv_vf
 */

#include "os/os_time.h"
#include "os/os_threading.h"

#include "util/u_trace_marker.h"
#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_format.h"
#include "util/u_frame.h"
#include "util/u_logging.h"
#include "util/u_trace_marker.h"

#include "vf_interface.h"

#include <stdio.h>
#include <assert.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/gstelement.h>
#include <gst/video/video-frame.h>

#define PL_LIBSOUP2


#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include <libsoup/soup-message.h>
#include <libsoup/soup-session.h>

#include <json-glib/json-glib.h>
#include "stdio.h"



/*
 *
 * Defines.
 *
 */

/*
 *
 * Printing functions.
 *
 */

#define VF_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define VF_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define VF_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define VF_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define VF_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)

DEBUG_GET_ONCE_LOG_OPTION(vf_log, "VF_LOG", U_LOGGING_TRACE)

/*!
 * A frame server operating on a video file.
 *
 * @implements xrt_frame_node
 * @implements xrt_fs
 */
struct vf_fs
{
	struct xrt_fs base;

	struct os_thread_helper play_thread;

	GMainLoop *loop;
	GstElement *source;
	GstElement *testsink;
	bool got_sample;
	int width;
	int height;
	enum xrt_format format;
	enum xrt_stereo_format stereo_format;

	struct xrt_frame_node node;

	struct
	{
		bool extended_format;
		bool timeperframe;
	} has;

	enum xrt_fs_capture_type capture_type;
	struct xrt_frame_sink *sink;

	uint32_t selected;

	struct xrt_fs_capture_parameters capture_params;

	bool is_configured;
	bool is_running;
	enum u_logging_level log_level;

	JavaVM *java_vm;
};

/*!
 * Frame wrapping a GstSample/GstBuffer.
 *
 * @implements xrt_frame
 */
struct vf_frame
{
	struct xrt_frame base;

	GstSample *sample;

	GstVideoFrame frame;
};



static gchar *websocket_uri = NULL;

static GOptionEntry options[] = {
    {"websocket-uri", 'u', 0, G_OPTION_ARG_STRING, &websocket_uri, "Websocket URI of webrtc signaling connection"},
    {NULL}};

#define WEBSOCKET_URI_DEFAULT "ws://127.0.0.1:8080/ws"

//!@todo Don't use global state
static SoupWebsocketConnection *ws = NULL;
static GstElement *pipeline = NULL;
static GstElement *webrtcbin = NULL;
static GstWebRTCDataChannel *datachannel = NULL;

//!@todo SORRY
static struct vf_fs *the_vf_fs = NULL;



/*
 *
 * Cast helpers.
 *
 */

/*!
 * Cast to derived type.
 */
static inline struct vf_fs *
vf_fs(struct xrt_fs *xfs)
{
	return (struct vf_fs *)xfs;
}

/*!
 * Cast to derived type.
 */
static inline struct vf_frame *
vf_frame(struct xrt_frame *xf)
{
	return (struct vf_frame *)xf;
}


/*
 *
 * Frame methods.
 *
 */

static void
vf_frame_destroy(struct xrt_frame *xf)
{
	SINK_TRACE_MARKER();

	struct vf_frame *vff = vf_frame(xf);

	gst_video_frame_unmap(&vff->frame);

	if (vff->sample != NULL) {
		gst_sample_unref(vff->sample);
		vff->sample = NULL;
	}

	free(vff);
}


/*
 *
 * Misc helper functions
 *
 */


static void
vf_fs_frame(struct vf_fs *vid, GstSample *sample)
{
	SINK_TRACE_MARKER();

	// Noop.
	if (!vid->sink) {
		return;
	}

	GstVideoInfo info;
	GstBuffer *buffer;
	GstCaps *caps;
	buffer = gst_sample_get_buffer(sample);
	caps = gst_sample_get_caps(sample);

	gst_video_info_init(&info);
	gst_video_info_from_caps(&info, caps);

	static int seq = 0;
	struct vf_frame *vff = U_TYPED_CALLOC(struct vf_frame);

	if (!gst_video_frame_map(&vff->frame, &info, buffer, GST_MAP_READ)) {
		VF_ERROR(vid, "Failed to map frame %d", seq);
		// Yes, we should do this here because we don't want the destroy function to run.
		free(vff);
		return;
	}

	// We now want to hold onto the sample for as long as the frame lives.
	gst_sample_ref(sample);
	vff->sample = sample;

	// Hardcoded first plane.
	int plane = 0;

	struct xrt_frame *xf = &vff->base;
	xf->reference.count = 1;
	xf->destroy = vf_frame_destroy;
	xf->width = vid->width;
	xf->height = vid->height;
	xf->format = vid->format;
	xf->stride = info.stride[plane];
	xf->data = vff->frame.data[plane];
	xf->stereo_format = vid->stereo_format;
	xf->size = info.size;
	xf->source_id = vid->base.source_id;

	//! @todo Proper sequence number and timestamp.
	xf->source_sequence = seq;
	xf->timestamp = os_monotonic_get_ns();

	xrt_sink_push_frame(vid->sink, &vff->base);

	xrt_frame_reference(&xf, NULL);
	vff = NULL;

	seq++;
}

static GstFlowReturn
on_new_sample_from_sink(GstElement *elt, struct vf_fs *vid)
{
	U_LOG_E("on_new_sample_from_sink called!");
	SINK_TRACE_MARKER();
	GstSample *sample;
	sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));

	if (!vid->got_sample) {
		gint width;
		gint height;

		GstCaps *caps = gst_sample_get_caps(sample);
		GstStructure *structure = gst_caps_get_structure(caps, 0);

		gst_structure_get_int(structure, "width", &width);
		gst_structure_get_int(structure, "height", &height);

		VF_DEBUG(vid, "video size is %dx%d", width, height);
		vid->got_sample = true;
		vid->width = width;
		vid->height = height;

		// first sample is only used for getting metadata
		return GST_FLOW_OK;
	}

	if (!vid->is_running) {
		U_LOG_E("We're not running, so we're not returning a sample");
		// I don't know if we can pause WebRTC stuff, so let's just not pause it
		return GST_FLOW_OK;
	}

	U_LOG_E("Going to try to return a sample");


	// Takes ownership of the sample.
	vf_fs_frame(vid, sample);

	// Done with sample now.
	gst_sample_unref(sample);

	return GST_FLOW_OK;
}

static void
print_gst_error(GstMessage *message)
{
	GError *err = NULL;
	gchar *dbg_info = NULL;

	gst_message_parse_error(message, &err, &dbg_info);
	U_LOG_E("ERROR from element %s: %s", GST_OBJECT_NAME(message->src), err->message);
	U_LOG_E("Debugging info: %s", (dbg_info) ? dbg_info : "none");
	g_error_free(err);
	g_free(dbg_info);
}

static gboolean
on_source_message(GstBus *bus, GstMessage *message, struct vf_fs *vid)
{
	/* nil */
	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_EOS:
		VF_DEBUG(vid, "Finished playback.");
		g_main_loop_quit(vid->loop);
		break;
	case GST_MESSAGE_ERROR:
		VF_ERROR(vid, "Received error.");
		print_gst_error(message);
		// g_main_loop_quit(vid->loop);
		break;
	default: break;
	}
	return TRUE;
}

static void *
vf_fs_mainloop(void *ptr)
{
	SINK_TRACE_MARKER();

	struct vf_fs *vid = (struct vf_fs *)ptr;

	VF_DEBUG(vid, "Let's run!");
	g_main_loop_run(vid->loop);
	VF_DEBUG(vid, "Going out!");

	gst_object_unref(vid->testsink);
	gst_element_set_state(vid->source, GST_STATE_NULL);


	gst_object_unref(vid->source);
	g_main_loop_unref(vid->loop);

	return NULL;
}



static gboolean
sigint_handler(gpointer user_data)
{
	U_LOG_E("sigint_handler called!");
	g_main_loop_quit(user_data);
	return G_SOURCE_REMOVE;
}

static gboolean
gst_bus_cb(GstBus *bus, GstMessage *message, gpointer data)
{
	U_LOG_E("gst_bus_cb called!");
	GstBin *pipeline = GST_BIN(data);

	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_ERROR: {
		GError *gerr;
		gchar *debug_msg;
		gst_message_parse_error(message, &gerr, &debug_msg);
		GST_DEBUG_BIN_TO_DOT_FILE(pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "mss-pipeline-ERROR");
		g_error("Error: %s (%s)", gerr->message, debug_msg);
		g_error_free(gerr);
		g_free(debug_msg);
	} break;
	case GST_MESSAGE_WARNING: {
		GError *gerr;
		gchar *debug_msg;
		gst_message_parse_warning(message, &gerr, &debug_msg);
		GST_DEBUG_BIN_TO_DOT_FILE(pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "mss-pipeline-WARNING");
		g_warning("Warning: %s (%s)", gerr->message, debug_msg);
		g_error_free(gerr);
		g_free(debug_msg);
	} break;
	case GST_MESSAGE_EOS: {
		g_error("Got EOS!!");
	} break;
	default: break;
	}
	return TRUE;
}

void
send_sdp_answer(const gchar *sdp)
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
	soup_websocket_connection_send_text(ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);
}

static void
webrtc_on_ice_candidate_cb(GstElement *webrtcbin, guint mlineindex, gchar *candidate)
{
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
	soup_websocket_connection_send_text(ws, msg_str);
	g_clear_pointer(&msg_str, g_free);

	json_node_unref(root);
	g_object_unref(builder);
}

static void
on_answer_created(GstPromise *promise, gpointer user_data)
{
	U_LOG_E("on_answer_created called!");
	GstWebRTCSessionDescription *answer = NULL;
	gchar *sdp;

	gst_structure_get(gst_promise_get_reply(promise), "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
	gst_promise_unref(promise);

    if (NULL == answer) {
        U_LOG_E("on_answer_created : ERROR !  get_promise answer = null !");
    }

	g_signal_emit_by_name(webrtcbin, "set-local-description", answer, NULL);

	sdp = gst_sdp_message_as_text(answer->sdp);
    if (NULL == sdp) {
        U_LOG_E("on_answer_created : ERROR !  sdp = null !");
    }
	send_sdp_answer(sdp);
	g_free(sdp);

	gst_webrtc_session_description_free(answer);
}

static void
process_sdp_offer(const gchar *sdp)
{
	U_LOG_E("process_sdp_offer called!");
	GstSDPMessage *sdp_msg = NULL;
	GstWebRTCSessionDescription *desc = NULL;


	U_LOG_E("Received offer: %s\n", sdp);

	if (gst_sdp_message_new_from_text(sdp, &sdp_msg) != GST_SDP_OK) {
		g_debug("Error parsing SDP description");
		goto out;
	}

	desc = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, sdp_msg);
	if (desc) {
		GstPromise *promise;

		promise = gst_promise_new();

		g_signal_emit_by_name(webrtcbin, "set-remote-description", desc, promise);

		gst_promise_wait(promise);
		gst_promise_unref(promise);

		g_signal_emit_by_name(
		    webrtcbin, "create-answer", NULL,
		    gst_promise_new_with_change_func((GstPromiseChangeFunc)on_answer_created, NULL, NULL));
	} else {
		gst_sdp_message_free(sdp_msg);
	}

out:
	g_clear_pointer(&desc, gst_webrtc_session_description_free);
}

static void
process_candidate(guint mlineindex, const gchar *candidate)
{
	U_LOG_E("process_candidate called!");
	U_LOG_E("Received candidate: %d %s\n", mlineindex, candidate);

	g_signal_emit_by_name(webrtcbin, "add-ice-candidate", mlineindex, candidate);
}

static void
message_cb(SoupWebsocketConnection *connection, gint type, GBytes *message, gpointer user_data)
{
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
			process_sdp_offer(offer_sdp);
		} else if (g_str_equal(msg_type, "candidate")) {
			JsonObject *candidate;

			candidate = json_object_get_object_member(msg, "candidate");

			process_candidate(json_object_get_int_member(candidate, "sdpMLineIndex"),
			                  json_object_get_string_member(candidate, "candidate"));
		}
	} else {
		g_debug("Error parsing message: %s", error->message);
		g_clear_error(&error);
	}

out:
	g_object_unref(parser);
}

// static void
// data_channel_error_cb(GstWebRTCDataChannel *datachannel, void *data)
// {
// 	U_LOG_E("error\n");
// 	abort();
// }

// static void
// data_channel_close_cb(GstWebRTCDataChannel *datachannel, gpointer timeout_src_id)
// {
// 	U_LOG_E("Data channel closed\n");

// 	g_source_remove(GPOINTER_TO_UINT(timeout_src_id));
// 	g_clear_object(&datachannel);
// }

// static void
// data_channel_message_string_cb(GstWebRTCDataChannel *datachannel, gchar *str, void *data)
// {
// 	U_LOG_E("Received data channel message: %s\n", str);
// }

// static gboolean
// datachannel_send_message(gpointer unused)
// {
// 	g_signal_emit_by_name(datachannel, "send-string", "Hi! from Pluto client");

// 	return G_SOURCE_CONTINUE;
// }

// static void
// webrtc_on_data_channel_cb(GstElement *webrtcbin, GstWebRTCDataChannel *data_channel, void *data)
// {
// 	guint timeout_src_id;

// 	U_LOG_E("Successfully created datachannel\n");

// 	g_assert_null(datachannel);

// 	datachannel = GST_WEBRTC_DATA_CHANNEL(data_channel);

// 	timeout_src_id = g_timeout_add_seconds(3, datachannel_send_message, NULL);

// 	g_signal_connect(datachannel, "on-close", G_CALLBACK(data_channel_close_cb), GUINT_TO_POINTER(timeout_src_id));
// 	g_signal_connect(datachannel, "on-error", G_CALLBACK(data_channel_error_cb), GUINT_TO_POINTER(timeout_src_id));
// 	g_signal_connect(datachannel, "on-message-string", G_CALLBACK(data_channel_message_string_cb), NULL);
// }


// You can generally figure these out by going to
// https://gstreamer.freedesktop.org/documentation/plugins_doc.html?gi-language=c, searching for the GstElement you
// need, and finding the plugin name it corresponds to in the sidebar. Some of them (libav is an example) don't
// correspond 1:1 in the names in the sidebar, so guessing as well as `find deps/gstreamer_android | grep <name>` helps.

GST_PLUGIN_STATIC_DECLARE(app); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(autodetect); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(nice);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(sctp);
GST_PLUGIN_STATIC_DECLARE(srtp);
GST_PLUGIN_STATIC_DECLARE(dtls);

//GST_PLUGIN_STATIC_DECLARE(usrsctp);
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
GST_PLUGIN_STATIC_DECLARE(webrtc);
GST_PLUGIN_STATIC_DECLARE(androidmedia);

GST_PLUGIN_STATIC_DECLARE(videotestsrc); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
GST_PLUGIN_STATIC_DECLARE(overlaycomposition);

GST_PLUGIN_STATIC_DECLARE(playback); // "FFMPEG "
// GST_PLUGIN_STATIC_DECLARE(webrtcnice);


static void
websocket_connected_cb(GObject *session, GAsyncResult *res, gpointer user_data)
{
	U_LOG_E("websocket_connected_cb called!");


GST_PLUGIN_STATIC_REGISTER(app); // Definitely needed
GST_PLUGIN_STATIC_REGISTER(autodetect); // Definitely needed
GST_PLUGIN_STATIC_REGISTER(coreelements);
GST_PLUGIN_STATIC_REGISTER(nice);
GST_PLUGIN_STATIC_REGISTER(rtp);
GST_PLUGIN_STATIC_REGISTER(rtpmanager);

//GST_PLUGIN_STATIC_REGISTER(usrsctp);
GST_PLUGIN_STATIC_REGISTER(sctp);
GST_PLUGIN_STATIC_REGISTER(srtp);
GST_PLUGIN_STATIC_REGISTER(dtls);
GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
GST_PLUGIN_STATIC_REGISTER(webrtc);
GST_PLUGIN_STATIC_REGISTER(androidmedia);

GST_PLUGIN_STATIC_REGISTER(videotestsrc); // Definitely needed
GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
GST_PLUGIN_STATIC_REGISTER(overlaycomposition);

GST_PLUGIN_STATIC_REGISTER(playback); // "FFMPEG "
// GST_PLUGIN_STATIC_REGISTER(webrtcnice);

	GError *error = NULL;

	g_assert(!ws);

	ws = soup_session_websocket_connect_finish(SOUP_SESSION(session), res, &error);
	if (error) {
		U_LOG_E("Error creating websocket: %s\n", error->message);
		g_clear_error(&error);
	} else {
		GstBus *bus;

		U_LOG_E("YO !! : Websocket connected\n");
		g_signal_connect(ws, "message", G_CALLBACK(message_cb), NULL);

		uint32_t width = 480;
		uint32_t height = 270;

		// decodebin3 seems to .. hang?
		// omxh264dec doesn't seem to exist

		gchar *pipeline_string = g_strdup_printf(
//            	"videotestsrc is-live=1 name=source ! "
				"webrtcbin name=webrtc bundle-policy=max-bundle ! rtph264depay ! amcviddec-omxqcomvideodecoderavc !"
//                "webrtcsrc start-call=false signaler::server_url=s://127.0.0.1:8080/ws ! rtph264depay ! decodebin3 !"
//				"videoconvertscale !"
				"appsink name=testsink caps=video/x-raw,format=(string)I420,width=%u,height=%u", width, height);
//                "appsink name=testsink caps=video/x-raw,format=RGB,width=%u,height=%u", width, height);



		printf("launching pipeline\n");
		pipeline = gst_parse_launch(pipeline_string, &error);

		if (error) {
			U_LOG_E("diff error");
			U_LOG_E("%s", error->message);
		}

		if (pipeline == NULL) {
			U_LOG_E("Bad source");
			U_LOG_E("%s", error->message);
			abort();
		}


		{
			struct vf_fs *vid = the_vf_fs;
			vid->source = pipeline;
			printf("getting testsink\n");
			vid->testsink = gst_bin_get_by_name(GST_BIN(vid->source), "testsink");
			printf("done getting testsink\n");
			g_object_set(G_OBJECT(vid->testsink), "emit-signals", TRUE, "sync", TRUE, NULL);
			g_signal_connect(vid->testsink, "new-sample", G_CALLBACK(on_new_sample_from_sink), vid);

			GstBus *bus = gst_element_get_bus(vid->source);
			gst_bus_add_watch(bus, (GstBusFunc)on_source_message, vid);
			gst_object_unref(bus);
		}

		g_assert_no_error(error);
		printf("getting webrtcbin\n");

		webrtcbin = gst_bin_get_by_name(GST_BIN(pipeline), "webrtc");
		printf("done getting webrtcbin\n");
		// g_signal_connect(webrtcbin, "on-data-channel", G_CALLBACK(webrtc_on_data_channel_cb), NULL);


		g_signal_connect(webrtcbin, "on-ice-candidate", G_CALLBACK(webrtc_on_ice_candidate_cb), NULL);

		bus = gst_element_get_bus(pipeline);
		gst_bus_add_watch(bus, gst_bus_cb, pipeline);
		gst_clear_object(&bus);

        U_LOG_E("YO : TRYING TO SET PLAY ");
		GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);

		if (ret == GST_STATE_CHANGE_FAILURE) {
			U_LOG_E("Noooo");

		} else {
			U_LOG_E("Successfully changed state!");
		}
	}
}

#if 0
int
blah(int argc, char *argv[])
{
	GOptionContext *option_context;
	GMainLoop *loop;
	SoupSession *soup_session;
	GError *error = NULL;

	gst_init(&argc, &argv);

	option_context = g_option_context_new(NULL);
	g_option_context_add_main_entries(option_context, options, NULL);

	if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
		U_LOG_E("option parsing failed: %s\n", error->message);
		exit(1);
	}

	if (!websocket_uri) {
		websocket_uri = g_strdup(WEBSOCKET_URI_DEFAULT);
	}

	soup_session = soup_session_new();

#ifdef PL_LIBSOUP2
	soup_session_websocket_connect_async(soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, websocket_uri), // message
	                                     NULL,                                             // origin
	                                     NULL,                                             // protocols
	                                     NULL,                                             // cancellable
	                                     websocket_connected_cb,                           // callback
	                                     NULL);                                            // user_data

#else
	soup_session_websocket_connect_async(soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, websocket_uri), // message
	                                     NULL,                                             // origin
	                                     NULL,                                             // protocols
	                                     0,                                                // io_prority
	                                     NULL,                                             // cancellable
	                                     websocket_connected_cb,                           // callback
	                                     NULL);                                            // user_data

#endif

	loop = g_main_loop_new(NULL, FALSE);
	g_unix_signal_add(SIGINT, sigint_handler, loop);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);
	g_clear_pointer(&websocket_uri, g_free);
}
#endif


/*
 *
 * Frame server methods.
 *
 */

static bool
vf_fs_enumerate_modes(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count)
{
	struct vf_fs *vid = vf_fs(xfs);

	struct xrt_fs_mode *modes = U_TYPED_ARRAY_CALLOC(struct xrt_fs_mode, 1);
	if (modes == NULL) {
		return false;
	}

	modes[0].width = vid->width;
	modes[0].height = vid->height;
	modes[0].format = vid->format;
	modes[0].stereo_format = vid->stereo_format;

	*out_modes = modes;
	*out_count = 1;

	return true;
}

static bool
vf_fs_configure_capture(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp)
{
	// struct vf_fs *vid = vf_fs(xfs);
	//! @todo
	return false;
}

static bool
vf_fs_stream_start(struct xrt_fs *xfs,
                   struct xrt_frame_sink *xs,
                    enum xrt_fs_capture_type capture_type,
                   uint32_t descriptor_index)
{
	struct vf_fs *vid = vf_fs(xfs);

	vid->sink = xs;
	vid->is_running = true;
	vid->capture_type = capture_type;
	vid->selected = descriptor_index;

	// gst_element_set_state(vid->source, GST_STATE_PLAYING);

	VF_TRACE(vid, "info: Started!");

	// we're off to the races!
	return true;
}

static bool
vf_fs_stream_stop(struct xrt_fs *xfs)
{
	struct vf_fs *vid = vf_fs(xfs);

	if (!vid->is_running) {
		return true;
	}

	vid->is_running = false;
	gst_element_set_state(vid->source, GST_STATE_PAUSED);

	return true;
}

static bool
vf_fs_is_running(struct xrt_fs *xfs)
{
	struct vf_fs *vid = vf_fs(xfs);

	GstState current = GST_STATE_NULL;
	GstState pending;
	gst_element_get_state(vid->source, &current, &pending, 0);

	return current == GST_STATE_PLAYING;
}

static void
vf_fs_destroy(struct vf_fs *vid)
{
	g_main_loop_quit(vid->loop);

	// Destroy also stops the thread.
	os_thread_helper_destroy(&vid->play_thread);

	free(vid);
}


/*
 *
 * Node methods.
 *
 */

static void
vf_fs_node_break_apart(struct xrt_frame_node *node)
{
	struct vf_fs *vid = container_of(node, struct vf_fs, node);
	vf_fs_stream_stop(&vid->base);
}

static void
vf_fs_node_destroy(struct xrt_frame_node *node)
{
	struct vf_fs *vid = container_of(node, struct vf_fs, node);
	vf_fs_destroy(vid);
}


/*
 *
 * Exported create functions and helper.
 *
 */

/*
GST_PLUGIN_STATIC_DECLARE(app); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(autodetect); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(nice);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(sctp);
GST_PLUGIN_STATIC_DECLARE(srtp);
GST_PLUGIN_STATIC_DECLARE(dtls);
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
GST_PLUGIN_STATIC_DECLARE(webrtc);
GST_PLUGIN_STATIC_DECLARE(androidmedia);
GST_PLUGIN_STATIC_DECLARE(videotestsrc); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
GST_PLUGIN_STATIC_DECLARE(overlaycomposition);
GST_PLUGIN_STATIC_DECLARE(playback); // "FFMPEG "
*/
static struct xrt_fs *
alloc_and_init_common(struct xrt_frame_context *xfctx,      //
                      enum xrt_format format,               //
                      enum xrt_stereo_format stereo_format) //
{
	struct vf_fs *vid = U_TYPED_CALLOC(struct vf_fs);
	the_vf_fs = vid;
	vid->got_sample = false;
	vid->format = format;
	vid->stereo_format = stereo_format;

	GstBus *bus = NULL;
/*
    GST_PLUGIN_STATIC_REGISTER(app); // Definitely needed
    GST_PLUGIN_STATIC_REGISTER(autodetect); // Definitely needed
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(nice);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(sctp);
    GST_PLUGIN_STATIC_REGISTER(srtp);
	GST_PLUGIN_STATIC_REGISTER(dtls);
    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
    GST_PLUGIN_STATIC_REGISTER(webrtc);
    GST_PLUGIN_STATIC_REGISTER(androidmedia);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc); // Definitely needed
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
    GST_PLUGIN_STATIC_REGISTER(overlaycomposition);

    GST_PLUGIN_STATIC_REGISTER(playback); // "FFMPEG "
*/
	int ret = os_thread_helper_init(&vid->play_thread);
	if (ret < 0) {
		VF_ERROR(vid, "Failed to init thread");
		// g_free(pipeline_string);
		free(vid);
		return NULL;
	}

	vid->loop = g_main_loop_new(NULL, FALSE);

	GOptionContext *option_context;
	GMainLoop *loop;
	SoupSession *soup_session;
	GError *error = NULL;

	option_context = g_option_context_new(NULL);
	g_option_context_add_main_entries(option_context, options, NULL);

	if (!g_option_context_parse(option_context, NULL, NULL, &error)) {
		U_LOG_E("option parsing failed: %s\n", error->message);
		exit(1);
	}

	if (!websocket_uri) {
		websocket_uri = g_strdup(WEBSOCKET_URI_DEFAULT);
	}

	soup_session = soup_session_new();

#ifdef PL_LIBSOUP2
	soup_session_websocket_connect_async(soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, websocket_uri), // message
	                                     NULL,                                             // origin
	                                     NULL,                                             // protocols
	                                     NULL,                                             // cancellable
	                                     websocket_connected_cb,                           // callback
	                                     NULL);                                            // user_data

#else
	soup_session_websocket_connect_async(soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, websocket_uri), // message
	                                     NULL,                                             // origin
	                                     NULL,                                             // protocols
	                                     0,                                                // io_prority
	                                     NULL,                                             // cancellable
	                                     websocket_connected_cb,                           // callback
	                                     NULL);                                            // user_data

#endif


#if 0
	vid->source = gst_parse_launch(pipeline_string, &error);
	g_free(pipeline_string);

	if (vid->source == NULL) {
		VF_ERROR(vid, "Bad source");
        VF_ERROR(vid, "%s", error->message);
		g_main_loop_unref(vid->loop);
		free(vid);
		return NULL;
	}

	vid->testsink = gst_bin_get_by_name(GST_BIN(vid->source), "testsink");
	g_object_set(G_OBJECT(vid->testsink), "emit-signals", TRUE, "sync", TRUE, NULL);
	g_signal_connect(vid->testsink, "new-sample", G_CALLBACK(on_new_sample_from_sink), vid);

	bus = gst_element_get_bus(vid->source);
	gst_bus_add_watch(bus, (GstBusFunc)on_source_message, vid);
	gst_object_unref(bus);
#endif

	ret = os_thread_helper_start(&vid->play_thread, vf_fs_mainloop, vid);
	if (ret != 0) {
		VF_ERROR(vid, "Failed to start thread '%i'", ret);
		g_main_loop_unref(vid->loop);
		free(vid);
		return NULL;
	}

#if 0
	// We need one sample to determine frame size.
	VF_DEBUG(vid, "Waiting for frame");
	{
		GstStateChangeReturn ret = gst_element_set_state(vid->source, GST_STATE_PLAYING);

		if (ret == GST_STATE_CHANGE_FAILURE) {
			VF_ERROR(vid, "WHAT.");
		}
	}
#endif
	// OK so it's stuck waiting here, alright.
	#if 0
	while (!vid->got_sample) {
		os_nanosleep(100 * 1000 * 1000);
	}
	#endif
	VF_DEBUG(vid, "Got first sample");
	// gst_element_set_state(vid->source, GST_STATE_PAUSED);

	vid->base.enumerate_modes = vf_fs_enumerate_modes;
	vid->base.configure_capture = vf_fs_configure_capture;
	vid->base.stream_start = vf_fs_stream_start;
	vid->base.stream_stop = vf_fs_stream_stop;
	vid->base.is_running = vf_fs_is_running;
	vid->node.break_apart = vf_fs_node_break_apart;
	vid->node.destroy = vf_fs_node_destroy;
	vid->log_level = debug_get_log_option_vf_log();

	// It's now safe to add it to the context.
	xrt_frame_context_add(xfctx, &vid->node);

	// Start the variable tracking after we know what device we have.
	// clang-format off
	u_var_add_root(vid, "Video File Frameserver", true);
	u_var_add_ro_text(vid, vid->base.name, "Card");
	u_var_add_log_level(vid, &vid->log_level, "Log Level");
	// clang-format on

	return &(vid->base);
}

struct xrt_fs *
vf_fs_videotestsource(struct xrt_frame_context *xfctx, uint32_t width, uint32_t height, JavaVM *java_vm)
{
	gst_init(0, NULL);

    gst_amc_jni_set_java_vm(java_vm);

	GST_DEBUG("lol");
	g_print("meow");

	enum xrt_format format = XRT_FORMAT_R8G8B8A8;
	enum xrt_stereo_format stereo_format = XRT_STEREO_FORMAT_NONE;

	// gchar *pipeline_string = g_strdup_printf(
	//     "videotestsrc is-live=1 name=source ! "
	//     "appsink name=testsink caps=video/x-raw,format=RGBx,width=%u,height=%u",
	//     width, height, width, height);

	return alloc_and_init_common(xfctx, format, stereo_format);
}
