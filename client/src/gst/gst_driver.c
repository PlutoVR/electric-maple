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
#include "gst_internal.h"
#include "em_connection.h"
#include "app_log.h"

#include "os/os_time.h"
#include "os/os_threading.h"

#include "util/u_trace_marker.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_format.h"
#include "util/u_frame.h"

#include "gst_common.h"

#include <GLES3/gl3.h>
#include <stdlib.h>
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
#include <gst/gstbus.h>
#include <gst/gstsample.h>
#include <gst/video/video-frame.h>
#include <gst/gl/gstglsyncmeta.h>

#include "gstjniutils.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


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
struct em_fs
{
	struct xrt_fs base;

	struct os_thread_helper play_thread;

	struct em_connection connection;

	GMainLoop *loop;
	GstElement *pipeline;
	GstGLDisplay *gst_gl_display;
	GstGLContext *gst_gl_context;
	GstGLContext *gst_gl_other_context;

	GstGLDisplay *display;
	/// Wrapped version of the android_main/render context
	GstGLContext *android_main_context;
	GstGLContext *context;
	GstElement *appsink;
	// GLenum texture_target; // WE SHOULD USE RENDER'S texture target
	// GLuint texture_id; // WE SHOULD USE RENDER'S texture id

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

	GLenum frame_texture_target;

	uint32_t selected;

	struct xrt_fs_capture_parameters capture_params;

	bool is_configured;
	bool is_running;
	enum u_logging_level log_level;

	struct em_state *state;
	JavaVM *java_vm;
};

/*!
 * Frame wrapping a GstSample/GstBuffer.
 *
 * @implements xrt_frame
 */
struct em_frame
{
	struct xrt_frame base;

	GstSample *sample;

	GstVideoFrame frame;
};


static gchar *websocket_uri = NULL;

#define WEBSOCKET_URI_DEFAULT "ws://127.0.0.1:8080/ws"

//!@todo Don't use global state
static GstWebRTCDataChannel *datachannel = NULL;

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

static GstBusSyncReply
bus_sync_handler_cb(GstBus *bus, GstMessage *msg, gpointer user_data)
{
	struct em_fs *vid = user_data;
	LOG_MSG(msg);

	/* Do not let GstGL retrieve the display handle on its own
	 * because then it believes it owns it and calls eglTerminate()
	 * when disposed */
	if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_NEED_CONTEXT) {
		const gchar *type;
		gst_message_parse_context_type(msg, &type);
		if (g_str_equal(type, GST_GL_DISPLAY_CONTEXT_TYPE)) {
			ALOGI("Got message: Need display context");
			g_autoptr(GstContext) context = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
			gst_context_set_gl_display(context, vid->display);
			gst_element_set_context(GST_ELEMENT(msg->src), context);
		} else if (g_str_equal(type, "gst.gl.app_context")) {
			ALOGI("Got message: Need app context");
			g_autoptr(GstContext) app_context = gst_context_new("gst.gl.app_context", TRUE);
			GstStructure *s = gst_context_writable_structure(app_context);
			gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, vid->android_main_context, NULL);
			gst_element_set_context(GST_ELEMENT(msg->src), app_context);
		}
	}

	return GST_BUS_PASS;
}

static void
render_gl_texture(struct em_fs *vid)
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
render_frame(struct em_fs *vid)
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
em_fs_mainloop(void *ptr)
{
	SINK_TRACE_MARKER();

	struct em_fs *vid = (struct em_fs *)ptr;

	ALOGD("em_fs_mainloop: running GMainLoop!");
	g_main_loop_run(vid->loop);
	ALOGD("em_fs_mainloop: g_main_loop_run returned, will unref");

	// gst_object_unref(vid->testsink);
	// gst_element_set_state(vid->source, GST_STATE_NULL);


	// gst_object_unref(vid->source);
	g_main_loop_unref(vid->loop);

	return NULL;
}

/*static void *
em_fs_mainloop(void *ptr)
{
  struct em_fs *vid = ptr;

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

/// Sets pipeline state to null, and stops+destroys the frame server thread
/// before cleaning up gstreamer objects
static void
stop_pipeline(struct em_fs *vid)
{
	vid->is_running = false;
	gst_element_set_state(vid->pipeline, GST_STATE_NULL);
	os_thread_helper_destroy(&vid->play_thread);
	em_connection_fini(&vid->connection);
	gst_clear_object(&vid->pipeline);
	gst_clear_object(&vid->display);
	gst_clear_object(&vid->android_main_context);
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
static inline struct em_fs *
em_fs(struct xrt_fs *xfs)
{
	return (struct em_fs *)xfs;
}

/*!
 * Cast to derived type.
 */
static inline struct em_frame *
em_frame(struct xrt_frame *xf)
{
	return (struct em_frame *)xf;
}


/*
 *
 * Frame methods.
 *
 */

static void
em_frame_destroy(struct xrt_frame *xf)
{
	SINK_TRACE_MARKER();

	struct em_frame *vff = em_frame(xf);

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
em_fs_frame(struct em_fs *vid, GstSample *sample)
{
	SINK_TRACE_MARKER();
	ALOGI("RYLIE: %s", __FUNCTION__);
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
	struct em_frame *vff = U_TYPED_CALLOC(struct em_frame);

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
	xf->destroy = em_frame_destroy;
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
on_source_message(GstBus *bus, GstMessage *message, struct em_fs *vid)
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
	struct em_fs *vid = user_data;
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

#define RYLIE "RYLIE: "

static void
em_init_gst_and_capture_context(struct em_fs *vid, EGLDisplay display, EGLContext context)
{

	ALOGI(RYLIE "wrapping egl context");

	EGLContext old_context = eglGetCurrentContext();
	EGLSurface old_read_surface = eglGetCurrentSurface(EGL_READ);
	EGLSurface old_draw_surface = eglGetCurrentSurface(EGL_DRAW);

	ALOGV(RYLIE "eglMakeCurrent make-current");

	// E_LOG_E("DEBUG: display=%p, surface=%p, ");
	//  We'll need and active egl context below
	if (eglMakeCurrent(display, EGL_NO_SURFACE /* vid->state->surface */, EGL_NO_SURFACE, context) == EGL_FALSE) {
		ALOGV(RYLIE "em_init_gst_and_capture_context: Failed make egl context current");
	}

	const GstGLPlatform egl_platform = GST_GL_PLATFORM_EGL;
	guintptr android_main_egl_context_handle = gst_gl_context_get_current_gl_context(egl_platform);
	GstGLAPI gl_api = gst_gl_context_get_current_gl_api(egl_platform, NULL, NULL);
	vid->gst_gl_display = g_object_ref_sink(gst_gl_display_new());
	vid->android_main_context = g_object_ref_sink(
	    gst_gl_context_new_wrapped(vid->gst_gl_display, android_main_egl_context_handle, egl_platform, gl_api));

	ALOGV(RYLIE "eglMakeCurrent un-make-current");
	eglMakeCurrent(display, old_draw_surface, old_read_surface, old_context);
}



// THE BELOW IS NICE FOR TESTING WITH THE ALTERNATIVE PIPELINE
// THE ONE WITH WEBRTCBIN ! APPSINK. TO MAKE SURE OUR WEBRTCBIN COMPONENT RECEIVES.. ANYTHING.
static GstFlowReturn
new_sample_cb(GstElement *appsink, gpointer data)
{
	g_autoptr(GstSample) sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
	ALOGE("YO: New sample %" GST_PTR_FORMAT "\n", sample);
	return GST_FLOW_OK;
}

// FOR RYAN:
// So we've successfully established a connection with the Server. (By the way,
// This has all been tested working on the Quest2). Of course, test this with your
// quest2 CONNECTED to laptop with USBc **and** CONNECTED TO THE SAME WIFI as your
// laptop ! :) . the signalling happens locally on 127.0.0.1 , but the ICE handshake
// (offers and candidates) will suggest a webrtc connection surely living on some
// 192.168.1.X IP adresses into your local WAN so yeah, be advised.

static GstElement *
launch_pipeline(gpointer user_data)
{

	GError *error = NULL;

	struct em_fs *vid = (struct em_fs *)user_data;
	{

		// decodebin3 seems to .. hang?
		// omxh264dec doesn't seem to exist

		uint32_t width = 480;
		uint32_t height = 270;

		// We'll need and active egl context below before setting up gstgl (as explained previously)
		ALOGE("FRED: websocket_connected_cb: Trying to get the EGL lock");
		os_mutex_lock(&vid->state->egl_lock);
		ALOGE("FRED : make current display=%p, surface=%p, context=%p", vid->state->display,
		      vid->state->surface, vid->state->context);
		if (eglMakeCurrent(vid->state->display, vid->state->surface, vid->state->surface,
		                   vid->state->context) == EGL_FALSE) {
			ALOGE("FRED: websocket_connected_cb: Failed make egl context current");
		}

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

		// And we unCurrent the egl context.
		eglMakeCurrent(vid->state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		ALOGE("FRED: websocket_connected: releasing the EGL lock");
		os_mutex_unlock(&vid->state->egl_lock);

		// We convert the string SINK_CAPS above into a GstCaps that elements below can understand.
		// the "video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY ")," part of the caps is read :
		// video/x-raw(memory:GLMemory) and is really important for getting zero-copy gl textures.
		// It tells the pipeline (especially the decoder) that an internal android:Surface should
		// get created internally (using the provided gstgl contexts above) so that the appsink
		// can basically pull the samples out using an GLConsumer (this is just for context, as
		// all of those constructs will be hidden from you, but are turned on by that CAPS).
		g_autoptr(GstCaps) caps = gst_caps_from_string(SINK_CAPS);

		// THIS SHOULD BE UNCOMMENTED ONLY WHEN TURNING THE TEST PIPELINE ON ABOVE
#if 0
		vid->appsink = gst_bin_get_by_name(GST_BIN(vid->pipeline), "appsink");
		gst_app_sink_set_emit_signals(GST_APP_SINK(vid->appsink), TRUE);
		g_signal_connect(vid->appsink, "new-sample", G_CALLBACK(new_sample_cb), NULL);
#else

		// THIS SHOULD BE COMMENTED WHEN TURNING THE TEST PIPELINE
		// FRED: We create the appsink 'manually' here because glsink's ALREADY a sink and so if we stick
		//       glsinkbin ! appsink in our pipeline_string for automatic linking, gst_parse will NOT like this,
		//       as glsinkbin (a sink) cannot link to anything upstream (appsink being 'another' sink). So we
		//       manually link them below using glsinkbin's 'sink' pad -> appsink.
		vid->appsink = gst_element_factory_make("appsink", NULL);
		g_object_set(vid->appsink, "caps", caps, NULL);
		g_autoptr(GstElement) glsinkbin = gst_bin_get_by_name(GST_BIN(vid->pipeline), "glsink");
		g_object_set(glsinkbin, "sink", vid->appsink, NULL);
#endif

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
		return vid->pipeline;
	}
}

/*
 *
 * Frame server methods.
 *
 */

static bool
em_fs_enumerate_modes(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count)
{
	struct em_fs *vid = em_fs(xfs);

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
em_fs_configure_capture(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp)
{
	// struct em_fs *vid = em_fs(xfs);
	//! @todo
	return false;
}

static bool
em_fs_stream_start(struct xrt_fs *xfs,
                   struct xrt_frame_sink *xs,
                   enum xrt_fs_capture_type capture_type,
                   uint32_t descriptor_index)
{
	struct em_fs *vid = em_fs(xfs);

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
em_fs_stream_stop(struct xrt_fs *xfs)
{
	struct em_fs *vid = em_fs(xfs);
	stop_pipeline(vid);
	return true;
}

static bool
em_fs_is_running(struct xrt_fs *xfs)
{
	struct em_fs *vid = em_fs(xfs);
	return vid->is_running;
}

static void
em_fs_destroy(struct em_fs *vid)
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
em_fs_node_break_apart(struct xrt_frame_node *node)
{
	struct em_fs *vid = container_of(node, struct em_fs, node);
	em_fs_stream_stop(&vid->base);
}

static void
em_fs_node_destroy(struct xrt_frame_node *node)
{
	struct em_fs *vid = container_of(node, struct em_fs, node);
	em_fs_destroy(vid);
}

/*
 *
 * Exported create functions and helper.
 *
 */

static struct xrt_fs *
alloc_and_init_common(struct xrt_frame_context *xfctx,
                      struct em_state *state,
                      enum xrt_format format,
                      enum xrt_stereo_format stereo_format,
                      EGLDisplay display,
                      EGLContext context)
{
	struct em_fs *vid = U_TYPED_CALLOC(struct em_fs);
	// the_em_fs = vid;
	// state->vid = vid;
	vid->format = format;
	vid->stereo_format = stereo_format;
	vid->state = state;

	GstBus *bus = NULL;

	ALOGE("FRED: alloc_and_init_common\n");

	em_init_gst_and_capture_context(vid, display, context);

	int ret = os_thread_helper_init(&vid->play_thread);
	if (ret < 0) {
		ALOGE("ERROR: Failed to init thread");
		// g_free(pipeline_string);
		free(vid);
		return NULL;
	}

	vid->loop = g_main_loop_new(NULL, FALSE);

	GError *error = NULL;

	if (!websocket_uri) {
		websocket_uri = g_strdup(WEBSOCKET_URI_DEFAULT);
	}

	em_connection_init(&vid->connection, launch_pipeline, vid, websocket_uri);

	em_connection_connect(&vid->connection);
	ALOGD("started async connect call, about to spawn em_fs_mainloop");


	ALOGE("FRED: Starting Play_thread");
	ret = os_thread_helper_start(&vid->play_thread, em_fs_mainloop, vid);
	if (ret != 0) {
		ALOGE("Failed to start thread '%i'", ret);
		g_main_loop_unref(vid->loop);
		free(vid);
		return NULL;
	}

	// Again, this is all em_fs (frameserver) stuff and might get ditched at some point.
	vid->base.enumerate_modes = em_fs_enumerate_modes;
	vid->base.configure_capture = em_fs_configure_capture;
	vid->base.stream_start = em_fs_stream_start;
	vid->base.stream_stop = em_fs_stream_stop;
	vid->base.is_running = em_fs_is_running;
	vid->node.break_apart = em_fs_node_break_apart;
	vid->node.destroy = em_fs_node_destroy;
	vid->log_level = debug_get_log_option_vf_log();

	// It's now safe to add it to the context.
	xrt_frame_context_add(xfctx, &vid->node);

	return &(vid->base);
}

// FOR RYAN : This is our entry point in the gst "Driver", a fancy way of saying
// we're entering C land. (take a look at gst_common.h's Extern "C" guarding
// declaration of this function when included from main.cpp.
struct xrt_fs *
em_fs_create_streaming_client(struct xrt_frame_context *xfctx,
                              struct em_state *state,
                              EGLDisplay display,
                              EGLContext context)
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

	g_print("meow");

	// The below calls are probably useless.. remnant of the xrt deps.
	enum xrt_format format = XRT_FORMAT_R8G8B8A8;
	enum xrt_stereo_format stereo_format = XRT_STEREO_FORMAT_NONE;

	return alloc_and_init_common(xfctx, state, format, stereo_format, display, context);
}

struct em_sample_impl
{
	struct em_sample base;
	GstSample *sample;
};

void
em_fs_release_sample(struct xrt_fs *fs, struct em_sample *ems)
{
	struct em_sample_impl *impl = (struct em_sample_impl *)ems;
	ALOGW("RYLIE: Releasing sample with texture ID %d", ems->frame_texture_id);
	gst_sample_unref(impl->sample);
	free(impl);
}

struct em_sample *
em_fs_try_pull_sample(struct xrt_fs *fs)
{

	// FOR RYAN : As mentioned, the glsinkbin -> appsink part of the gstreamer pipeline in gst_driver.c
	// will output "samples" that might be signalled for using "new_sample_cb" in gst_driver (useful for
	// testing if the gstreamer sink elements are giving us "anything"), but also on which we can "poll"
	// to see and that's what we do here. Note that the non-try version of the call "gst_app_sink_pull_sample"
	// WILL BLOCK until there's a sample.

	// Get Newest sample from GST appsink. Waiting 1ms here before giving up (might want to adjust that time)
	ALOGE("DEBUG: Trying to get new gstgl sample, waiting max 1ms\n");

	struct em_fs *vid = em_fs(fs);
	GstSample *sample =
	    gst_app_sink_try_pull_sample(GST_APP_SINK(vid->appsink), (GstClockTime)(1000 * GST_USECOND));

	if (sample == NULL) {
		return NULL;
	}

	ALOGE("FRED: GOT A SAMPLE !!!");
	GstBuffer *buffer = gst_sample_get_buffer(sample);
	GstCaps *caps = gst_sample_get_caps(sample);

	GstVideoInfo info;
	gst_video_info_from_caps(&info, caps);
	/*gint width = GST_VIDEO_INFO_WIDTH (&info);
	gint height = GST_VIDEO_INFO_HEIGHT (&info);*/

	// FOR RYAN: Handle resize according to how it's done in PlutosphereOXR
	/*if (width != vid->width || height != vid->height) {
	    vid->width = width;
	    vid->height = height;
	}*/

	struct em_sample_impl *ret = calloc(1, sizeof(struct em_sample_impl));

	GstVideoFrame frame;
	GstMapFlags flags = (GstMapFlags)(GST_MAP_READ | GST_MAP_GL);
	gst_video_frame_map(&frame, &info, buffer, flags);
	ret->base.frame_texture_id = *(GLuint *)frame.data[0];

	if (vid->context == NULL) {
		/* Get GStreamer's gl context. */
		gst_gl_query_local_gl_context(vid->appsink, GST_PAD_SINK, &vid->context);

		/* Check if we have 2D or OES textures */
		GstStructure *s = gst_caps_get_structure(caps, 0);
		const gchar *texture_target_str = gst_structure_get_string(s, "texture-target");
		if (g_str_equal(texture_target_str, GST_GL_TEXTURE_TARGET_EXTERNAL_OES_STR)) {
			vid->frame_texture_target = GL_TEXTURE_EXTERNAL_OES;
		} else if (g_str_equal(texture_target_str, GST_GL_TEXTURE_TARGET_2D_STR)) {
			vid->frame_texture_target = GL_TEXTURE_2D;
			ALOGE("RYLIE: Got GL_TEXTURE_2D instead of expected GL_TEXTURE_EXTERNAL_OES");
		} else {
			g_assert_not_reached();
		}
	}
	ret->base.frame_texture_target = vid->frame_texture_target;

	GstGLSyncMeta *sync_meta = gst_buffer_get_gl_sync_meta(buffer);
	if (sync_meta) {
		/* MOSHI: the set_sync() seems to be needed for resizing */
		gst_gl_sync_meta_set_sync_point(sync_meta, vid->context);
		gst_gl_sync_meta_wait(sync_meta, vid->context);
	}

	gst_video_frame_unmap(&frame);
	// gst_sample_unref(sample);
	// move sample ownership into the return value
	ret->sample = sample;
	return &(ret->base);
}
