// Copyright 2019-2023, Collabora, Ltd.
// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A GStreamer pipeline for WebRTC streaming
 * @author Moshi Turner <moses@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_misc.h"
#include "util/u_debug.h"

#include "gstreamer/gst_pipeline.h"

struct gstreamer_pipeline;
#ifdef __cplusplus
extern "C" {
#endif

void
gstreamer_webrtc_pipeline_play(struct gstreamer_pipeline *gp);

void
gstreamer_webrtc_pipeline_stop(struct gstreamer_pipeline *gp);

void
gstreamer_pipeline_webrtc_create(struct xrt_frame_context *xfctx,
                                 const char *appsrc_name,
                                 struct gstreamer_pipeline **out_gp);

#ifdef __cplusplus
}
#endif
