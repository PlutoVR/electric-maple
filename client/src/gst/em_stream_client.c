// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2022-2023, PlutoVR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pipeline module ElectricMaple XR streaming solution
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#include "em_stream_client.h"
#include "app_log.h"
#include "em_connection.h"

#include "os/os_threading.h"

#include <gst/app/gstappsink.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/gst.h>
#include <gst/gstbus.h>
#include <gst/gstelement.h>
#include <gst/gstinfo.h>
#include <gst/gstmessage.h>
#include <gst/gstsample.h>
#include <gst/gstutils.h>
#include <gst/video/video-frame.h>

#include <EGL/egl.h>


struct _EmStreamClient
{
	GMainLoop *loop;
	EmConnection *connection;
	GstElement *pipeline;
	GstGLDisplay *gst_gl_display;
	GstGLContext *gst_gl_context;
	GstGLContext *gst_gl_other_context;

	GstGLDisplay *display;

	/// Wrapped version of the android_main/render context
	GstGLContext *android_main_context;

	/// GStreamer-created EGL context for its own use
	GstGLContext *context;

	GstElement *appsink;

	struct
	{
		EGLDisplay display;
		EGLContext android_main_context;
		// 16x16 pbuffer surface
		EGLSurface surface;
	} egl;

	struct
	{
		EGLContext context;
		EGLSurface read_surface;
		EGLSurface draw_surface;
	} old_egl;

	struct os_thread_helper play_thread;

	// This mutex protects the EGL context below across main and gstgl threads
	struct os_mutex egl_lock;

	bool is_running;
};

G_DEFINE_TYPE(EmStreamClient, em_stream_client, G_TYPE_OBJECT);


enum
{
	// action signals
	// SIGNAL_CONNECT,
	// SIGNAL_DISCONNECT,
	// SIGNAL_SET_PIPELINE,
	// signals
	// SIGNAL_WEBSOCKET_CONNECTED,
	// SIGNAL_WEBSOCKET_FAILED,
	// SIGNAL_CONNECTED,
	// SIGNAL_STATUS_CHANGE,
	// SIGNAL_ON_NEED_PIPELINE,
	// SIGNAL_ON_DROP_PIPELINE,
	// SIGNAL_DISCONNECTED,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

typedef enum
{
	PROP_CONNECTION = 1,
	// PROP_STATUS,
	N_PROPERTIES
} EmStreamClientProperty;


// clang-format off
#define SINK_CAPS \
    "video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "              \
    "format = (string) RGBA, "                                          \
    "width = " GST_VIDEO_SIZE_RANGE ", "                                \
    "height = " GST_VIDEO_SIZE_RANGE ", "                               \
    "framerate = " GST_VIDEO_FPS_RANGE ", "                             \
    "texture-target = (string) { 2D, external-oes } "

// clang-format on


static void
on_need_pipeline_cb(EmConnection *emconn, EmStreamClient *sc);

static void
on_drop_pipeline_cb(EmConnection *emconn, EmStreamClient *sc);

/*
 * Helper functions
 */

static void
em_stream_client_egl_save(EmStreamClient *sc)
{
	sc->old_egl.context = eglGetCurrentContext();
	sc->old_egl.read_surface = eglGetCurrentSurface(EGL_READ);
	sc->old_egl.draw_surface = eglGetCurrentSurface(EGL_DRAW);
}

static void
em_stream_client_egl_restore(EmStreamClient *sc, EGLDisplay display)
{
	eglMakeCurrent(display, sc->old_egl.draw_surface, sc->old_egl.read_surface, sc->old_egl.context);
}


static bool
em_stream_client_egl_begin(EmStreamClient *sc)
{
	em_stream_client_egl_mutex_lock(sc);
	em_stream_client_egl_save(sc);
	ALOGE("%s : make current display=%p, surface=%p, context=%p", __FUNCTION__, sc->egl.display, sc->egl.surface,
	      sc->egl.android_main_context);
	if (eglMakeCurrent(sc->egl.display, sc->egl.surface, sc->egl.surface, sc->egl.android_main_context) ==
	    EGL_FALSE) {
		ALOGE("%s: Failed make egl context current", __FUNCTION__);
		em_stream_client_egl_mutex_unlock(sc);
		return false;
	}

	return true;
}

static void
em_stream_client_egl_end(EmStreamClient *sc)
{
	ALOGI("%s: Make egl context un-current", __FUNCTION__);
	em_stream_client_egl_restore(sc, sc->egl.display);
	em_stream_client_egl_mutex_unlock(sc);
}

/* GObject method implementations */

static void
em_stream_client_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	EmStreamClient *self = EM_STREAM_CLIENT(object);

	switch ((EmStreamClientProperty)property_id) {

	case PROP_CONNECTION:
		g_clear_object(&self->connection);
		self->connection = g_value_dup_object(value);

		g_signal_connect(self->connection, "on-need-pipeline", G_CALLBACK(on_need_pipeline_cb), self);
		g_signal_connect(self->connection, "on-drop-pipeline", G_CALLBACK(on_drop_pipeline_cb), self);
		ALOGI("EmConnection assigned");
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
	}
}

static void
em_stream_client_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	EmStreamClient *self = EM_STREAM_CLIENT(object);

	switch ((EmStreamClientProperty)property_id) {
	case PROP_CONNECTION: g_value_set_object(value, self->connection); break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
	}
}

static void
em_stream_client_init(EmStreamClient *sc)
{

	sc->loop = g_main_loop_new(NULL, FALSE);
	os_mutex_init(&sc->egl_lock);
}

static void
em_stream_client_dispose(GObject *object)
{
	EmStreamClient *self = EM_STREAM_CLIENT(object);
	g_clear_object(&self->connection);
	os_mutex_destroy(&self->egl_lock);
}

static void
em_stream_client_class_init(EmStreamClientClass *klass)
{
	gst_init(0, NULL);

	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = em_stream_client_dispose;

	gobject_class->set_property = em_stream_client_set_property;
	gobject_class->get_property = em_stream_client_get_property;

	/**
	 * EmStreamClient:connection:
	 *
	 * The websocket URI for the signaling server
	 */
	g_object_class_install_property(
	    gobject_class, PROP_CONNECTION,
	    g_param_spec_string("connection", "Connection", "EmConnection object for XR streaming",
	                        NULL /* default value */,
	                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}


static GstBusSyncReply
bus_sync_handler_cb(GstBus *bus, GstMessage *msg, EmStreamClient *sc)
{
	// LOG_MSG(msg);

	/* Do not let GstGL retrieve the display handle on its own
	 * because then it believes it owns it and calls eglTerminate()
	 * when disposed */
	if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_NEED_CONTEXT) {
		const gchar *type;
		gst_message_parse_context_type(msg, &type);
		if (g_str_equal(type, GST_GL_DISPLAY_CONTEXT_TYPE)) {
			ALOGI("Got message: Need display context");
			g_autoptr(GstContext) context = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
			gst_context_set_gl_display(context, sc->display);
			gst_element_set_context(GST_ELEMENT(msg->src), context);
		} else if (g_str_equal(type, "gst.gl.app_context")) {
			ALOGI("Got message: Need app context");
			g_autoptr(GstContext) app_context = gst_context_new("gst.gl.app_context", TRUE);
			GstStructure *s = gst_context_writable_structure(app_context);
			gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, sc->android_main_context, NULL);
			gst_element_set_context(GST_ELEMENT(msg->src), app_context);
		}
	}

	return GST_BUS_PASS;
}

/// Sets pipeline state to null, and stops+destroys the frame server thread
/// before cleaning up gstreamer objects
static void
stop_pipeline(EmStreamClient *sc)
{
	// sc->is_running = false;
	gst_element_set_state(sc->pipeline, GST_STATE_NULL);
	os_thread_helper_destroy(&sc->play_thread);
	g_clear_object(&sc->connection);
	gst_clear_object(&sc->pipeline);
	gst_clear_object(&sc->display);
	gst_clear_object(&sc->android_main_context);
	gst_clear_object(&sc->context);
}

static void
on_need_pipeline_cb(EmConnection *emconn, EmStreamClient *sc)
{

	GError *error = NULL;


	// decodebin3 seems to .. hang?
	// omxh264dec doesn't seem to exist

	uint32_t width = 480;
	uint32_t height = 270;

	// We'll need an active egl context below before setting up gstgl (as explained previously)
	ALOGE("FRED: websocket_connected_cb: Trying to get the EGL lock");
	em_stream_client_egl_mutex_lock(sc);
	ALOGE("FRED : make current display=%p, surface=%p, context=%p", sc->state->display, sc->state->surface,
	      sc->state->context);
	if (eglMakeCurrent(sc->state->display, sc->state->surface, sc->state->surface, sc->state->context) ==
	    EGL_FALSE) {
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
	if (NULL == sc) {
		ALOGE("FRED: OH ! NULL VID - this shouldn't happen !");
	}
	sc->pipeline = gst_object_ref_sink(gst_parse_launch(pipeline_string, &error));
	if (sc->pipeline == NULL) {
		ALOGE("FRED: Failed creating pipeline : Bad source: %s", error->message);
		abort();
	}

	// And we unCurrent the egl context.
	eglMakeCurrent(sc->state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	ALOGE("FRED: websocket_connected: releasing the EGL lock");
	os_mutex_unlock(&sc->state->egl_lock);

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
		sc->appsink = gst_bin_get_by_name(GST_BIN(sc->pipeline), "appsink");
		gst_app_sink_set_emit_signals(GST_APP_SINK(sc->appsink), TRUE);
		g_signal_connect(sc->appsink, "new-sample", G_CALLBACK(new_sample_cb), NULL);
#else

	// THIS SHOULD BE COMMENTED WHEN TURNING THE TEST PIPELINE
	// FRED: We create the appsink 'manually' here because glsink's ALREADY a sink and so if we stick
	//       glsinkbin ! appsink in our pipeline_string for automatic linking, gst_parse will NOT like this,
	//       as glsinkbin (a sink) cannot link to anything upstream (appsink being 'another' sink). So we
	//       manually link them below using glsinkbin's 'sink' pad -> appsink.
	sc->appsink = gst_element_factory_make("appsink", NULL);
	g_object_set(sc->appsink, "caps", caps, NULL);
	g_autoptr(GstElement) glsinkbin = gst_bin_get_by_name(GST_BIN(sc->pipeline), "glsink");
	g_object_set(glsinkbin, "sink", sc->appsink, NULL);
#endif

	g_autoptr(GstBus) bus = gst_element_get_bus(sc->pipeline);
	gst_bus_set_sync_handler(bus, bus_sync_handler_cb, sc, NULL);
	gst_bus_add_watch(bus, gst_bus_cb, sc->pipeline);
	g_object_unref(bus);

	// FOR RYAN : We are STARTING the pipeline. From this point forth, if built with
	// GST_DEBUG=*:6, you should see LOADS of GST output, including the webrtc negotiation.

	sc->is_running = TRUE;
	g_signal_emit_by_name(emconn, "set-pipeline", GST_PIPELINE(sc->pipeline), NULL);
}

static void
on_drop_pipeline_cb(EmConnection *emconn, EmStreamClient *sc)
{
	if (sc->pipeline) {
		gst_element_set_state(sc->pipeline, GST_STATE_NULL);
	}
	gst_clear_object(&sc->pipeline);
	gst_clear_object(&sc->appsink);
}

/*
 * Public functions
 */
EmStreamClient *
em_stream_client_new(EmConnection *connection)
{
	return EM_STREAM_CLIENT(g_object_new(EM_TYPE_STREAM_CLIENT, "connection", connection, NULL));
}

void
em_stream_client_set_egl_context(EmStreamClient *sc, EGLDisplay display, EGLContext context, EGLSurface pbuffer_surface)
{

	ALOGI("RYLIE: wrapping egl context");
	em_stream_client_egl_save(sc);

	ALOGV("RYLIE: eglMakeCurrent make-current");

	if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context) == EGL_FALSE) {
		ALOGV("RYLIE: em_stream_client_set_egl_context: Failed to make egl context current");
		return;
	}
	sc->egl.display = display;
	sc->egl.android_main_context = context;
	sc->egl.surface = pbuffer_surface;

	const GstGLPlatform egl_platform = GST_GL_PLATFORM_EGL;
	guintptr android_main_egl_context_handle = gst_gl_context_get_current_gl_context(egl_platform);
	GstGLAPI gl_api = gst_gl_context_get_current_gl_api(egl_platform, NULL, NULL);
	sc->gst_gl_display = g_object_ref_sink(gst_gl_display_new());
	sc->android_main_context = g_object_ref_sink(
	    gst_gl_context_new_wrapped(sc->gst_gl_display, android_main_egl_context_handle, egl_platform, gl_api));

	ALOGV("RYLIE: eglMakeCurrent un-make-current");
	em_stream_client_egl_restore(sc, display);
}

void
em_stream_client_egl_mutex_lock(EmStreamClient *sc)
{
	os_mutex_lock(&sc->egl_lock);
}

void
em_stream_client_egl_mutex_unlock(EmStreamClient *sc)
{
	os_mutex_unlock(&sc->egl_lock);
}
