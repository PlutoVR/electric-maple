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

DEBUG_GET_ONCE_LOG_OPTION(vf_log, "VF_LOG", U_LOGGING_WARN)

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
		g_main_loop_quit(vid->loop);
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

	gst_element_set_state(vid->source, GST_STATE_PLAYING);

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


GST_PLUGIN_STATIC_DECLARE(app); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(videotestsrc); // Definitely needed
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
GST_PLUGIN_STATIC_DECLARE(overlaycomposition);

static struct xrt_fs *
alloc_and_init_common(struct xrt_frame_context *xfctx,      //
                      enum xrt_format format,               //
                      enum xrt_stereo_format stereo_format, //
                      gchar *pipeline_string)               //
{
	struct vf_fs *vid = U_TYPED_CALLOC(struct vf_fs);
	vid->got_sample = false;
	vid->format = format;
	vid->stereo_format = stereo_format;

	GstBus *bus = NULL;

    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
    GST_PLUGIN_STATIC_REGISTER(overlaycomposition);

	int ret = os_thread_helper_init(&vid->play_thread);
	if (ret < 0) {
		VF_ERROR(vid, "Failed to init thread");
		g_free(pipeline_string);
		free(vid);
		return NULL;
	}

	vid->loop = g_main_loop_new(NULL, FALSE);
	VF_DEBUG(vid, "Pipeline: %s", pipeline_string);

    GError *error = NULL;

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

	ret = os_thread_helper_start(&vid->play_thread, vf_fs_mainloop, vid);
	if (ret != 0) {
		VF_ERROR(vid, "Failed to start thread '%i'", ret);
		g_main_loop_unref(vid->loop);
		free(vid);
		return NULL;
	}

	// We need one sample to determine frame size.
	VF_DEBUG(vid, "Waiting for frame");
    {
        GstStateChangeReturn ret = gst_element_set_state(vid->source, GST_STATE_PLAYING);

        if (ret == GST_STATE_CHANGE_FAILURE) {
            VF_ERROR(vid, "WHAT.");
        }
    }
	while (!vid->got_sample) {
		os_nanosleep(100 * 1000 * 1000);
	}
	VF_DEBUG(vid, "Got first sample");
	gst_element_set_state(vid->source, GST_STATE_PAUSED);

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
vf_fs_videotestsource(struct xrt_frame_context *xfctx, uint32_t width, uint32_t height)
{
	gst_init(0, NULL);

	enum xrt_format format = XRT_FORMAT_R8G8B8A8;
	enum xrt_stereo_format stereo_format = XRT_STEREO_FORMAT_NONE;

	gchar *pipeline_string = g_strdup_printf(
	    "videotestsrc name=source ! "
//	    "clockoverlay ! "
	    "videoconvertscale name=meow ! "
//	    "videoscale ! "
	    "video/x-raw,format=RGBA,width=%u,height=%u name=meow2 ! "
	    "appsink name=testsink",
	    width, height);

	return alloc_and_init_common(xfctx, format, stereo_format, pipeline_string);
}

struct xrt_fs *
vf_fs_open_file(struct xrt_frame_context *xfctx, const char *path)
{
	if (path == NULL) {
		U_LOG_E("No path given");
		return NULL;
	}

	gst_init(0, NULL);

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		U_LOG_E("File %s does not exist", path);
		return NULL;
	}

#if 0
	const gchar *caps = "video/x-raw,format=RGB";
	enum xrt_format format = XRT_FORMAT_R8G8B8;
	enum xrt_stereo_format stereo_format = XRT_STEREO_FORMAT_NONE;
#endif

#if 0
	// For hand tracking
	const gchar *caps = "video/x-raw,format=RGB";
	enum xrt_format format = XRT_FORMAT_R8G8B8;
	enum xrt_stereo_format stereo_format = XRT_STEREO_FORMAT_SBS;
#endif

#if 1
	const gchar *caps = "video/x-raw,format=YUY2";
	enum xrt_format format = XRT_FORMAT_YUYV422;
	enum xrt_stereo_format stereo_format = XRT_STEREO_FORMAT_SBS;
#endif

	gchar *loop = "false";

	gchar *pipeline_string = g_strdup_printf(
	    "multifilesrc location=\"%s\" loop=%s ! "
	    "decodebin ! "
	    "videoconvert ! "
	    "appsink caps=\"%s\" name=testsink",
	    path, loop, caps);

	return alloc_and_init_common(xfctx, format, stereo_format, pipeline_string);
}
