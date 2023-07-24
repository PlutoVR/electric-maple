// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  ElectricMaple XR streaming frameserver, based on video file frameserver implementation
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @ingroup xrt_fs_em
 */
#include "gst_common.h"
#include "app_log.h"

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

#include "gst_common.h"

#include <stdio.h>
#include <assert.h>

#include <glib.h>
#include <gst/app/gstappsink.h>
#include <gst/gl/gl.h>
#include <gst/gst.h>
#include <gst/gstelement.h>
#include <gst/gstinfo.h>
#include <gst/gstmessage.h>
#include <gst/gstutils.h>
#include <gst/video/video-frame.h>

#include "gstjniutils.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define PL_LIBSOUP2


#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include <libsoup/soup-message.h>
#include <libsoup/soup-session.h>

#include <json-glib/json-glib.h>


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


DEBUG_GET_ONCE_LOG_OPTION(vf_log, "VF_LOG", U_LOGGING_TRACE)

// clang-format off
#define SINK_CAPS \
    "video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "              \
    "format = (string) RGBA, "                                          \
    "width = " GST_VIDEO_SIZE_RANGE ", "                                \
    "height = " GST_VIDEO_SIZE_RANGE ", "                               \
    "framerate = " GST_VIDEO_FPS_RANGE ", "                             \
    "texture-target = (string) { 2D, external-oes } "

// clang-format on

/*!
 * A frame server for "Electric Maple" XR streaming
 *
 * @implements xrt_frame_node
 * @implements xrt_fs
 */

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

#define WEBSOCKET_URI_DEFAULT "ws://127.0.0.1:8080/ws"

//!@todo Don't use global state
static SoupWebsocketConnection *ws = NULL;
static GstElement *webrtcbin =
    NULL; // We need webrtcbin for the offer/promise management... no obvious way to pass 'vid' all the way through
static GstWebRTCDataChannel *datachannel = NULL;

//!@todo SORRY
// FIXME : Check if this was added for vid lifetime reasons.
static struct vf_fs *the_vf_fs = NULL;



void
em_gst_message_debug(const char *function, GstMessage *msg)
{
	const char *src_name = GST_MESSAGE_SRC_NAME(msg);
	const char *type_name = GST_MESSAGE_TYPE_NAME(msg);
	GstState old_state;
	GstState new_state;
	GstStreamStatusType stream_status;
	GstElement *owner = NULL;
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_STATE_CHANGED:
		gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
		ALOGI("%s: %s: %s: %s to %s", function, src_name, type_name, gst_element_state_get_name(old_state),
		      gst_element_state_get_name(new_state));
		break;
	case GST_MESSAGE_STREAM_STATUS:
		gst_message_parse_stream_status(msg, &stream_status, &owner);
		ALOGI("%s: %s: %s: %d", function, src_name, type_name, stream_status);
		break;
	default: ALOGI("%s: %s: %s", function, src_name, type_name);
	}
}
#define LOG_MSG(MSG)                                                                                                   \
	do {                                                                                                           \
		em_gst_message_debug(__FUNCTION__, MSG);                                                               \
	} while (0)


static GstBusSyncReply
bus_sync_handler_cb(GstBus *bus, GstMessage *msg, gpointer user_data)
{
	struct vf_fs *vid = user_data;

	/* Do not let GstGL retrieve the display handle on its own
	 * because then it believes it owns it and calls eglTerminate()
	 * when disposed */
	if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_NEED_CONTEXT) {
		const gchar *type;
		gst_message_parse_context_type(msg, &type);
		if (g_str_equal(type, GST_GL_DISPLAY_CONTEXT_TYPE)) {
			g_autoptr(GstContext) context = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
			gst_context_set_gl_display(context, vid->display);
			gst_element_set_context(GST_ELEMENT(msg->src), context);
		} else if (g_str_equal(type, "gst.gl.app_context")) {
			g_autoptr(GstContext) app_context = gst_context_new("gst.gl.app_context", TRUE);
			GstStructure *s = gst_context_writable_structure(app_context);
			gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, vid->other_context, NULL);
			gst_element_set_context(GST_ELEMENT(msg->src), app_context);
		}
	}

	return GST_BUS_PASS;
}

static void
render_gl_texture(struct vf_fs *vid)
{
	// FIXME: Render GL texture using vid->texture_target and vid->texture_id
#if 0
  glUseProgram(mProgramId);

  GLint positionLoc = glGetAttribLocation(mProgramId, "a_position");
  glEnableVertexAttribArray(positionLoc);
  glVertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, 0, pos);

  GLint texcoordLoc = glGetAttribLocation(mProgramId, "a_texcoord");
  glEnableVertexAttribArray(texcoordLoc);
  glVertexAttribPointer(texcoordLoc, 2, GL_FLOAT, GL_FALSE, 0, uv);

  GLint texLoc = glGetUniformLocation(mProgramId, "tex_sampler_0");
  glUniform1i(texLoc, 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(vid->texture_target, vid->texture_id);
  glTexParameteri(vid->texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(vid->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(vid->texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(vid->texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(positionLoc);
  glDisableVertexAttribArray(texcoordLoc);
  glBindTexture(vid->texture_target, 0);

  eglSwapBuffers(mEGLDisplay, mEGLSurface);
#endif
}

static gboolean
render_frame(struct vf_fs *vid)
{
	g_autoptr(GstSample) sample = gst_app_sink_pull_sample(GST_APP_SINK(vid->appsink));
	if (sample == NULL) {
		return FALSE;
	}

	GstBuffer *buffer = gst_sample_get_buffer(sample);
	GstCaps *caps = gst_sample_get_caps(sample);

	GstVideoInfo info;
	gst_video_info_from_caps(&info, caps);
	gint width = GST_VIDEO_INFO_WIDTH(&info);
	gint height = GST_VIDEO_INFO_HEIGHT(&info);
	if (width != vid->width || height != vid->height) {
		vid->width = width;
		vid->height = height;
		// FIXME: Handle resize
	}

	GstVideoFrame frame;
	gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ | GST_MAP_GL);
	vid->state->frame_texture_id = *(GLuint *)frame.data[0];

	if (vid->context == NULL) {
		/* Get GStreamer's gl context. */
		gst_gl_query_local_gl_context(vid->appsink, GST_PAD_SINK, &vid->context);

		/* Check if we have 2D or OES textures */
		GstStructure *s = gst_caps_get_structure(caps, 0);
		const gchar *texture_target_str = gst_structure_get_string(s, "texture-target");
		if (g_str_equal(texture_target_str, GST_GL_TEXTURE_TARGET_EXTERNAL_OES_STR)) {
			vid->state->frame_texture_target = GL_TEXTURE_EXTERNAL_OES;
		} else if (g_str_equal(texture_target_str, GST_GL_TEXTURE_TARGET_2D_STR)) {
			vid->state->frame_texture_target = GL_TEXTURE_2D;
		} else {
			g_assert_not_reached();
		}
	}

	GstGLSyncMeta *sync_meta = gst_buffer_get_gl_sync_meta(buffer);
	if (sync_meta) {
		/* XXX: the set_sync() seems to be needed for resizing */
		gst_gl_sync_meta_set_sync_point(sync_meta, vid->context);
		gst_gl_sync_meta_wait(sync_meta, vid->context);
	}

	// FIXME: Might not be necessary !
	// This will make our main renderer pick up and render the gl texture.
	vid->state->frame_available = true;

	gst_video_frame_unmap(&frame);

	return true;
}

static void *
vf_fs_mainloop(void *ptr)
{
	SINK_TRACE_MARKER();

	struct vf_fs *vid = (struct vf_fs *)ptr;

	ALOGD("Let's run!");
	g_main_loop_run(vid->loop);
	ALOGD("Going out!");

	// gst_object_unref(vid->testsink);
	// gst_element_set_state(vid->source, GST_STATE_NULL);


	// gst_object_unref(vid->source);
	g_main_loop_unref(vid->loop);

	return NULL;
}

/*static void *
vf_fs_mainloop(void *ptr)
{
  struct vf_fs *vid = ptr;

  SINK_TRACE_MARKER();

  eglBindAPI(EGL_OPENGL_ES_API);

  // FIXME: Take application GL context
  eglMakeCurrent(vid->state->display, vid->state->surface, vid->state->surface, vid->state->context);

  setup_sink(vid);

  GstStateChangeReturn ret = gst_element_set_state(vid->pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    ALOGE("Noooo");
  } else {
    ALOGE("Successfully changed state!");
  }

  while (vid->is_running) {
    if (!render_frame(vid)) {
      break;
    }
  }

  return NULL;
}*/

static void
stop_pipeline(struct vf_fs *vid)
{
	vid->is_running = false;
	gst_element_set_state(vid->pipeline, GST_STATE_NULL);
	os_thread_helper_destroy(&vid->play_thread);
	gst_clear_object(&vid->pipeline);
	gst_clear_object(&vid->display);
	gst_clear_object(&vid->other_context);
	gst_clear_object(&vid->context);
}

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
		ALOGE("Failed to map frame %d", seq);
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

static void
print_gst_error(GstMessage *message)
{
	GError *err = NULL;
	gchar *dbg_info = NULL;

	gst_message_parse_error(message, &err, &dbg_info);
	ALOGE("ERROR from element %s: %s", GST_OBJECT_NAME(message->src), err->message);
	ALOGE("Debugging info: %s", (dbg_info) ? dbg_info : "none");
	g_error_free(err);
	g_free(dbg_info);
}

static gboolean
on_source_message(GstBus *bus, GstMessage *message, struct vf_fs *vid)
{
	LOG_MSG(message);
	/* nil */
	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_EOS:
		ALOGD("Finished playback.");
		stop_pipeline(vid);
		break;
	case GST_MESSAGE_ERROR:
		ALOGE("Received error.");
		print_gst_error(message);
		stop_pipeline(vid);
		break;
	default: break;
	}
	return TRUE;
}

static gboolean
sigint_handler(gpointer user_data)
{
	struct vf_fs *vid = user_data;
	ALOGE("sigint_handler called!");
	stop_pipeline(vid);
	return G_SOURCE_REMOVE;
}

static gboolean
gst_bus_cb(GstBus *bus, GstMessage *message, gpointer data)
{
	LOG_MSG(message);

	GstBin *pipeline = GST_BIN(data);

	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_ERROR: {
		GError *gerr = NULL;
		gchar *debug_msg = NULL;
		gst_message_parse_error(message, &gerr, &debug_msg);
		GST_DEBUG_BIN_TO_DOT_FILE(pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-error");
		gchar *dotdata = gst_debug_bin_to_dot_data(pipeline, GST_DEBUG_GRAPH_SHOW_ALL);
		ALOGE("gst_bus_cb: DOT data: %s", dotdata);

		ALOGE("gst_bus_cb: Error: %s (%s)", gerr->message, debug_msg);
		g_error("gst_bus_cb: Error: %s (%s)", gerr->message, debug_msg);
		g_error_free(gerr);
		g_free(debug_msg);
	} break;
	case GST_MESSAGE_WARNING: {
		GError *gerr = NULL;
		gchar *debug_msg = NULL;
		gst_message_parse_warning(message, &gerr, &debug_msg);
		GST_DEBUG_BIN_TO_DOT_FILE(pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-warning");
		ALOGW("gst_bus_cb: Warning: %s (%s)", gerr->message, debug_msg);
		g_warning("gst_bus_cb: Warning: %s (%s)", gerr->message, debug_msg);
		g_error_free(gerr);
		g_free(debug_msg);
	} break;
	case GST_MESSAGE_EOS: {
		g_error("gst_bus_cb: Got EOS!!");
	} break;
	default: break;
	}
	return TRUE;
}

void
send_sdp_answer(const gchar *sdp)
{
	ALOGE("send_sdp_answer called!");
	JsonBuilder *builder;
	JsonNode *root;
	gchar *msg_str;

	ALOGE("Send answer: %s\n", sdp);

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
	ALOGE("webrtc_on_ice_candidate_cb called!");
	JsonBuilder *builder;
	JsonNode *root;
	gchar *msg_str;

	ALOGE("Send candidate: %u %s\n", mlineindex, candidate);

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
	ALOGE("on_answer_created called!");
	GstWebRTCSessionDescription *answer = NULL;
	gchar *sdp;

	gst_structure_get(gst_promise_get_reply(promise), "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
	gst_promise_unref(promise);

	if (NULL == answer) {
		ALOGE("on_answer_created : ERROR !  get_promise answer = null !");
	}

	g_signal_emit_by_name(webrtcbin, "set-local-description", answer, NULL);

	sdp = gst_sdp_message_as_text(answer->sdp);
	if (NULL == sdp) {
		ALOGE("on_answer_created : ERROR !  sdp = null !");
	}
	send_sdp_answer(sdp);
	g_free(sdp);

	gst_webrtc_session_description_free(answer);
}

static void
process_sdp_offer(const gchar *sdp)
{
	ALOGE("process_sdp_offer called!");
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
	ALOGE("process_candidate called!");
	ALOGE("Received candidate: %d %s\n", mlineindex, candidate);

	g_signal_emit_by_name(webrtcbin, "add-ice-candidate", mlineindex, candidate);
}

static void
message_cb(SoupWebsocketConnection *connection, gint type, GBytes *message, gpointer user_data)
{
	ALOGE("message_cb called!");
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
		ALOGE("Websocket message received: %s\n", msg_type);

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
// 	ALOGE("error\n");
// 	abort();
// }

// static void
// data_channel_close_cb(GstWebRTCDataChannel *datachannel, gpointer timeout_src_id)
// {
// 	ALOGE("Data channel closed\n");

// 	g_source_remove(GPOINTER_TO_UINT(timeout_src_id));
// 	g_clear_object(&datachannel);
// }

// static void
// data_channel_message_string_cb(GstWebRTCDataChannel *datachannel, gchar *str, void *data)
// {
// 	ALOGE("Received data channel message: %s\n", str);
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

// 	ALOGE("Successfully created datachannel\n");

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


// THE BELOW IS NICE FOR TESTING WITH THE ALTERNATIVE PIPELINE
// THE ONE WITH WEBRTCBIN ! APPSINK. TO MAKE SURE OUR WEBRTCBIN COMPONENT RECEIVES.. ANYTHING.
static GstFlowReturn
new_sample_cb(GstElement *appsink, gpointer data)
{
	g_autoptr(GstSample) sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
	ALOGE("YO: New sample %" GST_PTR_FORMAT "\n", sample);
	return GST_FLOW_OK;
}


// FOR RYAN : This is needed for all the GST_PLUGIN_STATIC_REGISTER
// counterparts below to work. Macro gstreamer code to easily declare
// and register all of the static plugins below. NOTE: ALL those
// might NOT be required, but that's what Moshi initially registered
// and I haven't had the time to check which were indeed requested
// for the gstreamer pipeline we're creating below.

// GST_PLUGIN_STATIC_DECLARE(app); // Definitely needed
// GST_PLUGIN_STATIC_DECLARE(autodetect); // Definitely needed
// GST_PLUGIN_STATIC_DECLARE(coreelements);
// GST_PLUGIN_STATIC_DECLARE(nice);
// GST_PLUGIN_STATIC_DECLARE(rtp);
// GST_PLUGIN_STATIC_DECLARE(rtpmanager);
// GST_PLUGIN_STATIC_DECLARE(sctp);
// GST_PLUGIN_STATIC_DECLARE(srtp);
// GST_PLUGIN_STATIC_DECLARE(dtls);
// GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
// GST_PLUGIN_STATIC_DECLARE(webrtc);
// GST_PLUGIN_STATIC_DECLARE(androidmedia);
// GST_PLUGIN_STATIC_DECLARE(opengl);
// GST_PLUGIN_STATIC_DECLARE(videotestsrc); // Definitely needed
// GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
// GST_PLUGIN_STATIC_DECLARE(overlaycomposition);
// GST_PLUGIN_STATIC_DECLARE(playback); // "FFMPEG "
// GST_PLUGIN_STATIC_DECLARE(webrtcnice);

// FOR RYAN:
// So we've successfully established a connection with the Server. (By the way,
// This has all been tested working on the Quest2). Of course, test this with your
// quest2 CONNECTED to laptop with USBc **and** CONNECTED TO THE SAME WIFI as your
// laptop ! :) . the signalling happens locally on 127.0.0.1 , but the ICE handshake
// (offers and candidates) will suggest a webrtc connection surely living on some
// 192.168.1.X IP adresses into your local WAN so yeah, be advised.
static void
websocket_connected_cb(GObject *session, GAsyncResult *res, gpointer user_data)
{
	ALOGE("Fred : websocket_connected_cb called!\n");

	// FOR RYAN : This is where our gstreamer plugins (especially the very important
	// "androidmedia" one - which is our hardware decoder on Quest2) get actually
	// registered and INITITIALIZED. all of them, except androidmedia, are self-
	// contained, but androidmedia will use JNI to call java classes, notably the
	// ones you'll find in the deps/gstreamer_android folder :
	//
	// deps/gstreamer_android/armv7/share/gst-android/ndk-build/androidmedia/GstAhcCallback.java
	// deps/gstreamer_android/armv7/share/gst-android/ndk-build/androidmedia/GstAhsCallback.java
	// deps/gstreamer_android/armv7/share/gst-android/ndk-build/androidmedia/GstAmcOnFrameAvailableListener.java
	//
	// so when integrating to PlutosphereOXR, please add those 3 java code to the project and
	// build as packaged UNDER the gstreamer.java package (explained in main.cpp) as :
	//
	// package: org.freedesktop.gstreamer.androidmedia
	// if properly built and integrated to your project, the below error message should NOT
	// be seen when gstreamer pipeline get STARTED and webrtc ICE is established :
	/* ERROR  [00m [00;04m             default gstjniutils.c:840:gst_amc_jni_get_application_class:[00m FRED:
	attempting to retrieve class org/freedesktop/gstreamer/androidmedia/GstAmcOnFrameAvailableListener 2023-06-23
	16:59:35.624  6433-6465  meow meow  com...ovr.plutosphere.webrtc_client  D  ** (<unknown>:6433): CRITICAL **:
	13:59:35.624: gst_amc_jni_object_local_unref: assertion 'object != NULL' failed 2023-06-23 16:59:35.624
	6433-6465  meow meow  com...ovr.plutosphere.webrtc_client  D  ** (<unknown>:6433): CRITICAL **: 13:59:35.624:
	gst_amc_jni_object_local_unref: assertion 'object != NULL' failed 2023-06-23 16:59:35.625  6433-6465  meow meow
	com...ovr.plutosphere.webrtc_client  D  0:00:02.415697812 [32m 6433[00m   0x7ba2308460 [31;01mERROR  [00m
	[00;04m             default
	gstamcsurfacetexture-jni.c:345:gst_amc_surface_texture_jni_set_on_frame_available_callback:[00m Could not
	create listener: Could not retrieve application class loader 2023-06-23 16:59:35.625  6433-6465  meow meow
	com...ovr.plutosphere.webrtc_client  D  0:00:02.415738281 [32m 6433[00m   0x7ba2308460 [33;01mWARN   [00m
	[00m         amcvideodec
	gstamcvideodec.c:2010:gst_amc_video_dec_set_format:<amcvideodec-omxqcomvideodecoderavc0>[00m error: Could not
	retrieve application class loader*/
	// GST_PLUGIN_STATIC_REGISTER(app); // Definitely needed
	// GST_PLUGIN_STATIC_REGISTER(autodetect); // Definitely needed
	// GST_PLUGIN_STATIC_REGISTER(coreelements);
	// GST_PLUGIN_STATIC_REGISTER(nice);
	// GST_PLUGIN_STATIC_REGISTER(rtp);
	// GST_PLUGIN_STATIC_REGISTER(rtpmanager);
	// GST_PLUGIN_STATIC_REGISTER(sctp);
	// GST_PLUGIN_STATIC_REGISTER(srtp);
	// GST_PLUGIN_STATIC_REGISTER(dtls);
	// GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
	// GST_PLUGIN_STATIC_REGISTER(webrtc);
	// GST_PLUGIN_STATIC_REGISTER(androidmedia);
	// GST_PLUGIN_STATIC_REGISTER(opengl);
	// GST_PLUGIN_STATIC_REGISTER(videotestsrc); // Definitely needed
	// GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
	// GST_PLUGIN_STATIC_REGISTER(overlaycomposition);
	// GST_PLUGIN_STATIC_REGISTER(playback); // "FFMPEG "
	// GST_PLUGIN_STATIC_REGISTER(webrtcnice);

	GError *error = NULL;

	g_assert(!ws);
	struct vf_fs *vid = (struct vf_fs *)user_data;

	ws = soup_session_websocket_connect_finish(SOUP_SESSION(session), res, &error);
	if (error) {
		ALOGE("Error creating websocket: %s\n", error->message);
		g_clear_error(&error);
	} else {
		ALOGE("YO !! : Websocket connected\n");
		g_signal_connect(ws, "message", G_CALLBACK(message_cb), NULL);

		// decodebin3 seems to .. hang?
		// omxh264dec doesn't seem to exist

		uint32_t width = 480;
		uint32_t height = 270;


		gchar *pipeline_string = g_strdup_printf(
		    "webrtcbin name=webrtc bundle-policy=max-bundle ! "
		    "rtph264depay ! "
		    "h264parse ! "
		    "video/x-h264,stream-format=(string)byte-stream, alignment=(string)au,parsed=(boolean)true !"
		    "amcviddec-omxqcomvideodecoderavc ! "
		    "glsinkbin name=glsink");

		// THIS IS THE TEST PIPELINE FOR TESTING WITH APPSINK AT VARIOUS PHASES OF THE PIPELINE AND
		// SEE IF YOU GET SAMPLES WITH THE BELOW g_signal_connect(..., new_sample_cb) signal.
		// for example, for testing that the h264parser gives you stuff :
		//                "webrtcbin name=webrtc bundle-policy=max-bundle ! rtph264depay ! h264parse ! appsink
		//                name=appsink");

		ALOGI("launching pipeline\n");
		if (NULL == vid) {
			ALOGE("FRED: OH ! NULL VID - this shouldn't happen !");
		}
		vid->pipeline = gst_object_ref_sink(gst_parse_launch(pipeline_string, &error));
		if (vid->pipeline == NULL) {
			ALOGE("FRED: Failed creating pipeline : Bad source");
			ALOGE("%s", error->message);
			abort();
		}

		ALOGI("getting webrtcbin\n");
		webrtcbin = gst_bin_get_by_name(GST_BIN(vid->pipeline), "webrtc");
		g_signal_connect(webrtcbin, "on-ice-candidate", G_CALLBACK(webrtc_on_ice_candidate_cb), NULL);

		// We'll need and active egl context below before setting up gstgl (as explained previously)
		ALOGE("FRED: websocket_connected_cb: Trying to get the EGL lock");
		os_mutex_lock(&vid->state->egl_lock);
		ALOGE("FRED : make current display=%i, surface=%i, context=%i", (int)vid->state->display,
		      (int)vid->state->surface, (int)vid->state->context);
		if (eglMakeCurrent(vid->state->display, vid->state->surface, vid->state->surface,
		                   vid->state->context) == EGL_FALSE) {
			ALOGE("FRED: websocket_connected_cb: Failed make egl context current");
		}

		// Important gstgl (opengl gstreamer plugin) considerations for the glsinkbin sink.
		// There is a very good example of a very similar project using gstreamer opengl
		// for the MagicLeap backend. You may read and inspire yourself.:
		//
		// https://gitlab.freedesktop.org/xclaesse/gstreamer_demo/-/blob/master/VideoScene.cpp
		//
		GstGLPlatform gl_platform = GST_GL_PLATFORM_EGL;
		guintptr gl_handle = gst_gl_context_get_current_gl_context(gl_platform);
		GstGLAPI gl_api = gst_gl_context_get_current_gl_api(gl_platform, NULL, NULL);
		vid->gst_gl_display = g_object_ref_sink(gst_gl_display_new());
		vid->other_context =
		    g_object_ref_sink(gst_gl_context_new_wrapped(vid->gst_gl_display, gl_handle, gl_platform, gl_api));

		// We convert the string SINK_CAPS above into a GstCaps that elements below can understand.
		// the "video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY ")," part of the caps is read :
		// video/x-raw(memory:GLMemory) and is really important for getting zero-copy gl textures.
		// It tells the pipeline (especially the decoder) that an internal android:Surface should
		// get created internally (using the provided gstgl contexts above) so that the appsink
		// can basically pull the samples out using an GLConsumer (this is just for context, as
		// all of those constructs will be hidden from you, but are turned on by that CAPS).
		g_autoptr(GstCaps) caps = gst_caps_from_string(SINK_CAPS);

		// THIS SHOULD BE UNCOMMENTED ONLY WHEN TURNING THE TEST PIPELINE ON ABOVE
		/*vid->appsink = gst_bin_get_by_name(GST_BIN(vid->pipeline), "appsink");
		gst_app_sink_set_emit_signals(GST_APP_SINK(vid->appsink), TRUE);
		g_signal_connect(vid->appsink, "new-sample", G_CALLBACK(new_sample_cb), NULL);*/

		// THIS SHOULD BE COMMENTED WHEN TURNING THE TEST PIPELINE
		// FRED: We create the appsink 'manually' here because glsink's ALREADY a sink and so if we stick
		// glsinkbin ! appsink
		//       in our pipeline_string for automatic linking, gst_parse will NOT like this, as glsinkbin (a
		//       sink) cannot link to anything upstream (appsink being 'another' sink). So we manually link them
		//       below using glsinkbin's 'sink' pad -> appsink.
		vid->appsink = gst_element_factory_make("appsink", NULL);
		g_object_set(vid->appsink, "caps", caps, NULL);
		g_autoptr(GstElement) glsinkbin = gst_bin_get_by_name(vid->pipeline, "glsink");
		g_object_set(glsinkbin, "sink", vid->appsink, NULL);

		g_autoptr(GstBus) bus = gst_element_get_bus(vid->pipeline);
		gst_bus_set_sync_handler(bus, bus_sync_handler_cb, vid, NULL);
		gst_bus_add_watch(bus, gst_bus_cb, vid->pipeline);
		g_object_unref(bus);

		// FOR RYAN : We are STARTING the pipeline. From this point forth, if built with
		// GST_DEBUG=*:6, you should see LOADS of GST output, including the webrtc negotiation.
		gst_element_set_state(vid->pipeline, GST_STATE_PLAYING);

		// FIXME: Implement this when implementing data channel
		// g_signal_connect(webrtcbin, "on-data-channel", G_CALLBACK(webrtc_on_data_channel_cb), NULL);

		vid->is_running = TRUE;

		// And we unCurrent the egl context.
		eglMakeCurrent(vid->state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		ALOGE("FRED: websocket_connected: releasing the EGL lock");
		os_mutex_unlock(&vid->state->egl_lock);
	}
}

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

	ALOGV("info: Started!");

	// we're off to the races!
	return true;
}

static bool
vf_fs_stream_stop(struct xrt_fs *xfs)
{
	struct vf_fs *vid = vf_fs(xfs);
	stop_pipeline(vid);
	return true;
}

static bool
vf_fs_is_running(struct xrt_fs *xfs)
{
	struct vf_fs *vid = vf_fs(xfs);
	return vid->is_running;
}

static void
vf_fs_destroy(struct vf_fs *vid)
{
	stop_pipeline(vid);
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

static struct xrt_fs *
alloc_and_init_common(struct xrt_frame_context *xfctx,      //
                      struct state_t *state,                //
                      enum xrt_format format,               //
                      enum xrt_stereo_format stereo_format) //
{
	struct vf_fs *vid = U_TYPED_CALLOC(struct vf_fs);
	the_vf_fs = vid;
	state->vid = vid;
	vid->format = format;
	vid->stereo_format = stereo_format;
	vid->state = state;

	GstBus *bus = NULL;

	ALOGE("FRED: alloc_and_init_common\n");

	// FOR RYAN: We'll need this thread/mainloop for the websocket cb to fire below.
	int ret = os_thread_helper_init(&vid->play_thread);
	if (ret < 0) {
		ALOGE("ERROR: Failed to init thread");
		// g_free(pipeline_string);
		free(vid);
		return NULL;
	}

	vid->loop = g_main_loop_new(NULL, FALSE);

	GOptionContext *option_context;
	SoupSession *soup_session;
	GError *error = NULL;

	if (!websocket_uri) {
		websocket_uri = g_strdup(WEBSOCKET_URI_DEFAULT);
	}

	soup_session = soup_session_new();

	// FOR RYAN : We're using libsoup here to create our websocket session. We're starting the thread herebelow
	// on play_thread . You might wanna change how it's handled, but basically it's needed for
	// websocket_connected_cb to fire when the connected's made. and that's where code flows continues...
#ifdef PL_LIBSOUP2
	ALOGE("FRED: calling soup_session_websocket_connect_async. websocket_uri = %s\n", websocket_uri);
	soup_session_websocket_connect_async(soup_session,                                     // session
	                                     soup_message_new(SOUP_METHOD_GET, websocket_uri), // message
	                                     NULL,                                             // origin
	                                     NULL,                                             // protocols
	                                     NULL,                                             // cancellable
	                                     websocket_connected_cb,                           // callback
	                                     vid);                                             // user_data
#endif

	ALOGE("FRED: Starting Play_thread");
	ret = os_thread_helper_start(&vid->play_thread, vf_fs_mainloop, vid);
	if (ret != 0) {
		ALOGE("Failed to start thread '%i'", ret);
		g_main_loop_unref(vid->loop);
		free(vid);
		return NULL;
	}

	// Again, this is all vf_fs (frameserver) stuff and might get ditched at some point.
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

// FOR RYAN : This is our entry point in the gst "Driver", a fancy way of saying
// we're entering C land. (take a look at gst_common.h's Extern "C" guarding
// declaration of this function when included from main.cpp.
struct xrt_fs *
vf_fs_gst_pipeline(struct xrt_frame_context *xfctx, struct state_t *state)
{
	// FOR RYAN: The below JNI_OnLoad call MUST NOT be called !! It's just
	// a hint to tell you how gstreamer will get inited from an Java/Native android app
	// such as PlutosphereOXR (not the case here).
	// In the case of PlutosphereOXR, libgstreamer-1.0 will HAVE to be linked as a SHARED
	// library (it's the case here, look into one of the CMakeLists.txt I have and you will
	// have to LOAD that library from Java (like from main activity's OnCreate() function)
	// with the below call :
	//
	// System.LoadLibrary(gstreamer-1.0)
	//
	// This LoadLibrary function will internally call the library's JNI_OnLoad(...) function
	// that will register all the JNI native functions so that you can thereafter use the
	// gstreamer's androidmedia plugin that makes use of JNI calls internally.
	//
	// For more context, read important documentation about this here :
	// https://developer.android.com/training/articles/perf-jni#native-libraries
	//
	// IMPORTANT :
	// When Calling System.LoadlLibrary(...) however, internal JNI code will try to
	// retrieve some Java classes throught he Java Loader..and those classes as defined
	// in GStreamer.java that you'll find in the gstreamer_android folder (under deps/)
	// It's important that you ADD that GStreamer.java code to your project as a new
	// org.freedesktop.gstreamer package for it to be "findable" by the libgstreamer-1.0
	// loader.

	// DO NOT CALL THIS : JNI_OnLoad(state->java_vm, NULL);

	// gst init. Make sure this gets calls AFTER Java-side System.LoadLibrary(gstreamer-1.0)
	gst_init(0, NULL);

	// This "might" be optional in Plutosphere Integration, but I would probably
	// start by KEEPING it, it's a convenient call that GIVES the current Java VM to the
	// Androidmedia plugin (amc = androidmediacodec) so it DOES NOT have to deal with the
	// InvocationAPI internally when initializing and can just safely rely on the JavaVM
	// we've given it.
	// for more context, see amc gstreamer code :
	// https://github.com/GStreamer/gst-plugins-bad/blob/master/sys/androidmedia/gstamc.c
	// and here is what it's doing with the provided JAVA VM :
	// https://github.com/GStreamer/gst-plugins-bad/blob/master/sys/androidmedia/gstjniutils.c#L783
	// gst_amc_jni_set_java_vm(state->java_vm);

	g_print("meow");

	// The below calls are probably useless.. remnant of rhte xrt deps.
	enum xrt_format format = XRT_FORMAT_R8G8B8A8;
	enum xrt_stereo_format stereo_format = XRT_STEREO_FORMAT_NONE;

	// Actually init gst plugins and create gstreamer pipeline !!
	return alloc_and_init_common(xfctx, state, format, stereo_format);
}
