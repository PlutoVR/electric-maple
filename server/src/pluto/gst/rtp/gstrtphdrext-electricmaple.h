/* GStreamer
 * Copyright (C) 2020-2021, 2023 Collabora Ltd.
 * Copyright (C) 2023 Pluto VR, Inc.
 *   @author: Ryan Pavlik <rpavlik@collabora.com>
 *   @author: Jakub Adam <jakub.adam@collabora.com>
 *
 * gstrtphdrext-electricmaple.h: Electric Maple XR streaming RTP header extension
 *
 * SPDX-License-Identifier: LGPL-2-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_RTPHDREXT_ELECTRICMAPLE_H__
#define __GST_RTPHDREXT_ELECTRICMAPLE_H__

/* Based on gstrtphdrext-colorspace.h */

#include <gst/rtp/gstrtphdrext.h>

G_BEGIN_DECLS
#define GST_EM_FRAME_ID_VIDEO_META_API_TYPE (gst_em_frame_id_video_meta_api_get_type())
/**
 * GST_EM_FRAME_ID_VIDEO_META_INFO:
 *
 * The #GstMetaInfo associated with #GstEMFrameIdVideoMeta.
 *
 * Since: 1.20
 */
#define GST_EM_FRAME_ID_VIDEO_META_INFO (gst_em_frame_id_video_meta_get_info())
typedef struct _GstEMFrameIdVideoMeta GstEMFrameIdVideoMeta;

struct _GstEMFrameIdVideoMeta
{
	GstMeta meta;

	guint64 frame_id;
};

GType
gst_em_frame_id_video_meta_api_get_type(void);

const GstMetaInfo *
gst_em_frame_id_video_meta_get_info(void);

GstEMFrameIdVideoMeta *
gst_buffer_add_em_frame_id_video_meta(GstBuffer *buffer, guint64 frame_id);


GstEMFrameIdVideoMeta *
gst_buffer_get_em_frame_id_video_meta(GstBuffer *buffer);

#define GST_RTP_HDREXT_ELECTRIC_MAPLE_SIZE 8
// #define GST_RTP_HDREXT_ELECTRICMAPLE_SIZE 28
// #define GST_RTP_HDREXT_ELECTRICMAPLE_URI "http://www.webrtc.org/experiments/rtp-hdrext/color-space"

#define GST_RTP_HDREXT_ELECTRIC_MAPLE_URI "http://gitlab.collabora.com/rpavlik/electric-maple-rtc/branches/20231004"

#define GST_TYPE_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE (gst_rtp_header_extension_electric_maple_get_type())

G_DECLARE_FINAL_TYPE(GstRTPHeaderExtensionElectricMaple,
                     gst_rtp_header_extension_electric_maple,
                     GST,
                     RTP_HEADER_EXTENSION_ELECTRIC_MAPLE,
                     GstRTPHeaderExtension)

G_END_DECLS

#endif /* __GST_RTPHDREXT_ELECTRICMAPLE_H__ */
