// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Implementation of sample
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */
#include "em_sample.h"

#include "em_app_log.h"

#include <gst/video/video-frame.h>
#include <gst/gl/gstglbasememory.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglsyncmeta.h>

#include <stdlib.h>


EmSample *
em_sample_new(GstSample *gst_sample, GstGLContext *context)
{
	if (gst_sample == NULL) {
		return NULL;
	}
	ALOGV("%s: Retrieving buffer and caps for %p", __FUNCTION__, gst_sample);
	GstBuffer *buffer = gst_sample_get_buffer(gst_sample);
	GstCaps *caps = gst_sample_get_caps(gst_sample);

	GstVideoInfo info;
	gst_video_info_from_caps(&info, caps);
	GstVideoFrame frame;

	ALOGV("%s: Mapping buffer for %p", __FUNCTION__, gst_sample);
	gst_video_frame_map(&frame, &info, buffer, (GstMapFlags)(GST_MAP_READ | GST_MAP_GL));

	ALOGV("%s: Creating EmSample for %p", __FUNCTION__, gst_sample);

	EmSample *ret = calloc(1, sizeof(EmSample));
	ret->sample = gst_sample_ref(gst_sample);

	ret->frame_texture_id = *(GLuint *)frame.data[0];

	if (context) {
		GstGLSyncMeta *sync_meta = gst_buffer_get_gl_sync_meta(buffer);
		if (sync_meta) {
			/* MOSHI: the set_sync() seems to be needed for resizing */
			ALOGV("%s: Syncing GL on %p", __FUNCTION__, gst_sample);

			gst_gl_sync_meta_set_sync_point(sync_meta, context);
			gst_gl_sync_meta_wait(sync_meta, context);
		}
	}
	return ret;
}

EmSample *
em_sample_copy(EmSample *s)
{
	if (s == NULL) {
		return NULL;
	}
	EmSample *ret = calloc(1, sizeof(EmSample));
	ret->frame_texture_id = s->frame_texture_id;
	// ret->frame_texture_target = s->frame_texture_target;
	ret->sample = gst_sample_ref(s->sample);
	return ret;
}

void
em_sample_free(EmSample *s)
{
	if (s == NULL) {
		return;
	}
	gst_clear_object(&s->sample);
	free(s);
}

G_DEFINE_BOXED_TYPE(EmSample, em_sample, em_sample_copy, em_sample_free)
