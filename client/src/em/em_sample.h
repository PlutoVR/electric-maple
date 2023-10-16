// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Header for sample
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#pragma once

#include <GLES3/gl3.h>
#include <gst/gstsample.h>
#include <gst/gl/gstgl_fwd.h>

typedef struct _EmSample
{
	GLuint frame_texture_id;
	GstSample *sample;
	gint width;
	gint height;
	// GLenum frame_texture_target;
} EmSample;

G_BEGIN_DECLS


EmSample *
em_sample_new(GstSample *gst_sample, GstGLContext *context);

EmSample *
em_sample_copy(EmSample *s);

void
em_sample_free(EmSample *s);

G_END_DECLS
