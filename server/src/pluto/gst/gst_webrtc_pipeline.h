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

#ifdef __cplusplus
extern "C" {
#endif

struct gstreamer_pipeline;
struct pl_callbacks;

typedef struct _pluto_DownMessage pluto_DownMessage;

void
gstreamer_webrtc_pipeline_set_down_msg(struct gstreamer_pipeline *gp, pluto_DownMessage *msg);

void
gstreamer_webrtc_pipeline_play(struct gstreamer_pipeline *gp);

void
gstreamer_webrtc_pipeline_stop(struct gstreamer_pipeline *gp);

void
gstreamer_pipeline_webrtc_create(struct xrt_frame_context *xfctx,
                                 const char *appsrc_name,
                                 struct pl_callbacks *callbacks_collection,
                                 struct gstreamer_pipeline **out_gp);

#ifdef __cplusplus
}
#endif
