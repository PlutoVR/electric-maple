/* GStreamer
 * Copyright (C) 2020-2021, 2023 Collabora Ltd.
 * Copyright (C) 2023 Pluto VR, Inc.
 *   @author: Ryan Pavlik <rpavlik@collabora.com>
 *   @author: Jakub Adam <jakub.adam@collabora.com>
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

/**
 * SECTION:rtphdrextcolorspace
 * @title: GstRtphdrext-Colorspace
 * @short_description: Helper methods for dealing with Color Space RTP header
 * extension as defined in  http://www.webrtc.org/experiments/rtp-hdrext/color-space
 * @see_also: #GstRTPHeaderExtension, #GstRTPBasePayload, #GstRTPBaseDepayload
 *
 * Since: 1.20
 */

#include "gstrtphdrext-electricmaple.h"

// #include "gstrtpelements.h"

#include <gst/base/gstbytereader.h>

GST_DEBUG_CATEGORY_STATIC(rtphdrext_electric_maple_debug);
#define GST_CAT_DEFAULT (rtphdrext_electric_maple_debug)

GType
gst_em_frame_id_video_meta_api_get_type(void)
{
	static GType type = 0;
	static const gchar *tags[] = {NULL};

	if (g_once_init_enter(&type)) {
		GType _type = gst_meta_api_type_register("GstEMFrameIdVideoMetaAPI", tags);
		g_once_init_leave(&type, _type);
	}
	return type;
}

static gboolean
gst_em_frame_id_video_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
	GstEMFrameIdVideoMeta *dmeta = (GstEMFrameIdVideoMeta *)meta;

	dmeta->frame_id = 0;

	return TRUE;
}

static gboolean
gst_em_frame_id_video_meta_transform(GstBuffer *dst, GstMeta *meta, GstBuffer *src, GQuark type, gpointer data)
{
	if (GST_META_TRANSFORM_IS_COPY(type)) {
		GstEMFrameIdVideoMeta *smeta = (GstEMFrameIdVideoMeta *)meta;
		GstEMFrameIdVideoMeta *dmeta;

		dmeta = gst_buffer_add_em_frame_id_video_meta(dst, smeta->frame_id);
		if (dmeta == NULL)
			return FALSE;
	} else {
		/* return FALSE, if transform type is not supported */
		return FALSE;
	}

	return TRUE;
}

const GstMetaInfo *
gst_em_frame_id_video_meta_get_info(void)
{
	static const GstMetaInfo *em_frame_id_video_meta_info = NULL;

	if (g_once_init_enter(&em_frame_id_video_meta_info)) {
		const GstMetaInfo *meta = gst_meta_register(
		    GST_EM_FRAME_ID_VIDEO_META_API_TYPE, "GstEMFrameIdVideoMeta", sizeof(GstEMFrameIdVideoMeta),
		    gst_em_frame_id_video_meta_init, (GstMetaFreeFunction)NULL, gst_em_frame_id_video_meta_transform);
		g_once_init_leave(&em_frame_id_video_meta_info, meta);
	}
	return em_frame_id_video_meta_info;
}

GstEMFrameIdVideoMeta *
gst_buffer_add_em_frame_id_video_meta(GstBuffer *buffer, guint64 frame_id)
{
	GstEMFrameIdVideoMeta *meta;

	g_return_val_if_fail(buffer != NULL, NULL);

	meta = (GstEMFrameIdVideoMeta *)gst_buffer_add_meta(buffer, GST_EM_FRAME_ID_VIDEO_META_INFO, NULL);
	if (!meta)
		return NULL;

	meta->frame_id = frame_id;

	return meta;
}

GstEMFrameIdVideoMeta *
gst_buffer_get_em_frame_id_video_meta(GstBuffer *buffer)
{
	return (GstEMFrameIdVideoMeta *)gst_buffer_get_meta(buffer, gst_em_frame_id_video_meta_api_get_type());
}

/**
 * GstRTPHeaderExtensionElectricMaple:
 * @parent: the parent #GstRTPHeaderExtension
 *
 * Instance struct for Color Space RTP header extension.
 *
 * http://www.webrtc.org/experiments/rtp-hdrext/color-space
 */
struct _GstRTPHeaderExtensionElectricMaple
{
	GstRTPHeaderExtension parent;

	// GstVideoColorimetry colorimetry;
	// GstVideoChromaSite chroma_site;
	// GstVideoMasteringDisplayInfo mdi;
	// GstVideoContentLightLevel cll;
	// gboolean has_hdr_meta;
};

G_DEFINE_TYPE_WITH_CODE(
    GstRTPHeaderExtensionElectricMaple,
    gst_rtp_header_extension_electric_maple,
    GST_TYPE_RTP_HEADER_EXTENSION,
    GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "rtphdrextelectricmaple", 0, "RTP Electric Maple Header Extension"));
GST_ELEMENT_REGISTER_DEFINE(rtphdrextelectricmaple,
                            "rtphdrextelectricmaple",
                            GST_RANK_MARGINAL,
                            GST_TYPE_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE);

static void
gst_rtp_header_extension_electric_maple_init(GstRTPHeaderExtensionElectricMaple *self)
{}

static GstRTPHeaderExtensionFlags
gst_rtp_header_extension_electric_maple_get_supported_flags(GstRTPHeaderExtension *ext)
{
	// GstRTPHeaderExtensionElectricMaple *self = GST_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE(ext);

	return GST_RTP_HEADER_EXTENSION_ONE_BYTE;
}

static gsize
gst_rtp_header_extension_electric_maple_get_max_size(GstRTPHeaderExtension *ext, const GstBuffer *buffer)
{
	// GstRTPHeaderExtensionElectricMaple *self =
	//     GST_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE (ext);

	return GST_RTP_HDREXT_ELECTRIC_MAPLE_SIZE;
}

static gssize
gst_rtp_header_extension_electric_maple_write(GstRTPHeaderExtension *ext,
                                              const GstBuffer *input_meta,
                                              GstRTPHeaderExtensionFlags write_flags,
                                              GstBuffer *output,
                                              guint8 *data,
                                              gsize size)
{
	// GstRTPHeaderExtensionElectricMaple *self =
	//     GST_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE (ext);
	GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
	// gboolean is_frame_last_buffer;
	guint8 *ptr = data;
	// guint8 range;
	// guint8 horizontal_site;
	// guint8 vertical_site;

	// g_return_val_if_fail (size >=
	//     gst_rtp_header_extension_electric_maple_get_max_size (ext, NULL), -1);
	// g_return_val_if_fail (write_flags &
	//     gst_rtp_header_extension_electric_maple_get_supported_flags (ext), -1);

	// if (self->colorimetry.matrix == GST_VIDEO_COLOR_MATRIX_UNKNOWN &&
	//     self->colorimetry.primaries == GST_VIDEO_COLOR_PRIMARIES_UNKNOWN &&
	//     self->colorimetry.range == GST_VIDEO_COLOR_RANGE_UNKNOWN &&
	//     self->colorimetry.transfer == GST_VIDEO_TRANSFER_UNKNOWN) {
	//   /* Nothing to write. */
	//   return 0;
	// }

	// gst_rtp_buffer_map (output, GST_MAP_READ, &rtp);
	// is_frame_last_buffer = gst_rtp_buffer_get_marker (&rtp);
	// gst_rtp_buffer_unmap (&rtp);

	// if (!is_frame_last_buffer) {
	//   /* Only a video frame's final packet should carry color space info. */
	//   return 0;
	// }

	// *ptr++ = gst_video_color_primaries_to_iso (self->colorimetry.primaries);
	// *ptr++ = gst_video_transfer_function_to_iso (self->colorimetry.transfer);
	// *ptr++ = gst_video_color_matrix_to_iso (self->colorimetry.matrix);

	// switch (self->colorimetry.range) {
	//   case GST_VIDEO_COLOR_RANGE_0_255:
	//     range = 2;
	//     break;
	//   case GST_VIDEO_COLOR_RANGE_16_235:
	//     range = 1;
	//     break;
	//   default:
	//     range = 0;
	//     break;
	// }

	// if (self->chroma_site & GST_VIDEO_CHROMA_SITE_H_COSITED) {
	//   horizontal_site = 1;
	// } else if (self->chroma_site & GST_VIDEO_CHROMA_SITE_NONE) {
	//   horizontal_site = 2;
	// } else {
	//   horizontal_site = 0;
	// }

	// if (self->chroma_site & GST_VIDEO_CHROMA_SITE_V_COSITED) {
	//   vertical_site = 1;
	// } else if (self->chroma_site & GST_VIDEO_CHROMA_SITE_NONE) {
	//   vertical_site = 2;
	// } else {
	//   vertical_site = 0;
	// }

	// *ptr++ = (range << 4) + (horizontal_site << 2) + vertical_site;

	// if (self->has_hdr_meta) {
	//   guint i;

	//   GST_WRITE_UINT16_BE (ptr,
	//       self->mdi.max_display_mastering_luminance / 10000);
	//   ptr += 2;
	//   GST_WRITE_UINT16_BE (ptr, self->mdi.min_display_mastering_luminance);
	//   ptr += 2;

	//   for (i = 0; i < 3; ++i) {
	//     GST_WRITE_UINT16_BE (ptr, self->mdi.display_primaries[i].x);
	//     ptr += 2;
	//     GST_WRITE_UINT16_BE (ptr, self->mdi.display_primaries[i].y);
	//     ptr += 2;
	//   }

	//   GST_WRITE_UINT16_BE (ptr, self->mdi.white_point.x);
	//   ptr += 2;
	//   GST_WRITE_UINT16_BE (ptr, self->mdi.white_point.y);
	//   ptr += 2;

	//   GST_WRITE_UINT16_BE (ptr, self->cll.max_content_light_level);
	//   ptr += 2;
	//   GST_WRITE_UINT16_BE (ptr, self->cll.max_frame_average_light_level);
	//   ptr += 2;
	// }

	return ptr - data;
}

// TODO need outparams
static gboolean
parse_electric_maple(GstByteReader *reader)
{
	// guint8 val;

	// g_return_val_if_fail (reader != NULL, FALSE);
	// g_return_val_if_fail (colorimetry != NULL, FALSE);
	// g_return_val_if_fail (chroma_site != NULL, FALSE);

	// if (gst_byte_reader_get_remaining (reader) < GST_RTP_HDREXT_electric_maple_SIZE) {
	//   return FALSE;
	// }

	// if (!gst_byte_reader_get_uint8 (reader, &val)) {
	//   return FALSE;
	// }
	// colorimetry->primaries = gst_video_color_primaries_from_iso (val);

	// if (!gst_byte_reader_get_uint8 (reader, &val)) {
	//   return FALSE;
	// }
	// colorimetry->transfer = gst_video_transfer_function_from_iso (val);

	// if (!gst_byte_reader_get_uint8 (reader, &val)) {
	//   return FALSE;
	// }
	// colorimetry->matrix = gst_video_color_matrix_from_iso (val);

	// *chroma_site = GST_VIDEO_CHROMA_SITE_UNKNOWN;

	// if (!gst_byte_reader_get_uint8 (reader, &val)) {
	//   return FALSE;
	// }
	// switch ((val >> 2) & 0x03) {
	//   case 1:
	//     *chroma_site |= GST_VIDEO_CHROMA_SITE_H_COSITED;
	//     break;
	//   case 2:
	//     *chroma_site |= GST_VIDEO_CHROMA_SITE_NONE;
	//     break;
	// }

	// switch (val & 0x03) {
	//   case 1:
	//     *chroma_site |= GST_VIDEO_CHROMA_SITE_V_COSITED;
	//     break;
	//   case 2:
	//     *chroma_site |= GST_VIDEO_CHROMA_SITE_NONE;
	//     break;
	// }

	// switch (val >> 4) {
	//   case 1:
	//     colorimetry->range = GST_VIDEO_COLOR_RANGE_16_235;
	//     break;
	//   case 2:
	//     colorimetry->range = GST_VIDEO_COLOR_RANGE_0_255;
	//     break;
	//   default:
	//     colorimetry->range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
	//     break;
	// }

	return TRUE;
}

static gboolean
gst_rtp_header_extension_electric_maple_read(GstRTPHeaderExtension *ext,
                                             GstRTPHeaderExtensionFlags read_flags,
                                             const guint8 *data,
                                             gsize size,
                                             GstBuffer *buffer)
{
	GstRTPHeaderExtensionElectricMaple *self = GST_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE(ext);
	// gboolean has_hdr_meta;
	GstByteReader *reader;
	// GstVideoColorimetry colorimetry;
	// GstVideoChromaSite chroma_site;
	// GstVideoMasteringDisplayInfo mdi;
	// GstVideoContentLightLevel cll;
	// gboolean caps_update_needed;
	gboolean result;

	if (size != GST_RTP_HDREXT_ELECTRIC_MAPLE_SIZE) {
		GST_WARNING_OBJECT(ext, "Invalid Electric Maple header extension size %" G_GSIZE_FORMAT, size);
		return FALSE;
	}

	// has_hdr_meta = size == GST_RTP_HDREXT_electric_maple_WITH_HDR_META_SIZE;

	reader = gst_byte_reader_new(data, size);

	// if (has_hdr_meta) {
	//   result = parse_electric_maple_with_hdr_meta (reader, &colorimetry, &chroma_site,
	//       &mdi, &cll);
	// } else {
	// TODO need outparams
	result = parse_electric_maple(reader);
	// }

	// something like gst_buffer_add_audio_level_meta

	// g_clear_pointer (&reader, gst_byte_reader_free);

	// if (!gst_video_colorimetry_is_equal (&self->colorimetry, &colorimetry)) {
	//   caps_update_needed = TRUE;
	//   self->colorimetry = colorimetry;
	// }

	// if (self->chroma_site != chroma_site) {
	//   caps_update_needed = TRUE;
	//   self->chroma_site = chroma_site;
	// }

	// if (self->has_hdr_meta != has_hdr_meta) {
	//   caps_update_needed = TRUE;
	//   self->has_hdr_meta = has_hdr_meta;
	// }

	// if (has_hdr_meta) {
	//   if (!gst_video_mastering_display_info_is_equal (&self->mdi, &mdi)) {
	//     caps_update_needed = TRUE;
	//     self->mdi = mdi;
	//   }
	//   if (!gst_video_content_light_level_is_equal (&self->cll, &cll)) {
	//     caps_update_needed = TRUE;
	//     self->cll = cll;
	//   }
	// }

	// if (caps_update_needed) {
	//   gst_rtp_header_extension_set_wants_update_non_rtp_src_caps (ext, TRUE);
	// }

	return result;
}

static gboolean
gst_rtp_header_extension_electric_maple_set_non_rtp_sink_caps(GstRTPHeaderExtension *ext, const GstCaps *caps)
{
	// GstRTPHeaderExtensionElectricMaple *self =
	//     GST_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE (ext);
	// GstStructure *s;
	// const gchar *colorimetry;
	// const gchar *chroma_site;

	// s = gst_caps_get_structure (caps, 0);

	// colorimetry = gst_structure_get_string (s, "colorimetry");
	// if (colorimetry) {
	//   gst_video_colorimetry_from_string (&self->colorimetry, colorimetry);

	//   self->has_hdr_meta =
	//       gst_video_mastering_display_info_from_caps (&self->mdi, caps);

	//   gst_video_content_light_level_from_caps (&self->cll, caps);
	// }

	// chroma_site = gst_structure_get_string (s, "chroma-site");
	// if (chroma_site) {
	//   self->chroma_site = gst_video_chroma_from_string (chroma_site);
	// }

	return TRUE;
}

static gboolean
gst_rtp_header_extension_electric_maple_update_non_rtp_src_caps(GstRTPHeaderExtension *ext, GstCaps *caps)
{
	// GstRTPHeaderExtensionElectricMaple *self =
	//     GST_RTP_HEADER_EXTENSION_ELECTRIC_MAPLE (ext);

	// gchar *color_str;

	// gst_structure_remove_fields (gst_caps_get_structure (caps, 0),
	//     "mastering-display-info", "content-light-level", NULL);

	// if ((color_str = gst_video_colorimetry_to_string (&self->colorimetry))) {
	//   gst_caps_set_simple (caps, "colorimetry", G_TYPE_STRING, color_str, NULL);
	//   g_free (color_str);
	// }
	// if (self->chroma_site != GST_VIDEO_CHROMA_SITE_UNKNOWN) {
	//   gst_caps_set_simple (caps, "chroma-site", G_TYPE_STRING,
	//       gst_video_chroma_to_string (self->chroma_site), NULL);
	// }
	// if (self->has_hdr_meta) {
	//   gst_video_mastering_display_info_add_to_caps (&self->mdi, caps);
	//   gst_video_content_light_level_add_to_caps (&self->cll, caps);
	// }

	return TRUE;
}

static void
gst_rtp_header_extension_electric_maple_class_init(GstRTPHeaderExtensionElectricMapleClass *klass)
{
	GstRTPHeaderExtensionClass *rtp_hdr_class = GST_RTP_HEADER_EXTENSION_CLASS(klass);
	GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

	rtp_hdr_class->get_supported_flags = gst_rtp_header_extension_electric_maple_get_supported_flags;
	rtp_hdr_class->get_max_size = gst_rtp_header_extension_electric_maple_get_max_size;
	rtp_hdr_class->write = gst_rtp_header_extension_electric_maple_write;
	rtp_hdr_class->read = gst_rtp_header_extension_electric_maple_read;
	rtp_hdr_class->set_non_rtp_sink_caps = gst_rtp_header_extension_electric_maple_set_non_rtp_sink_caps;
	rtp_hdr_class->update_non_rtp_src_caps = gst_rtp_header_extension_electric_maple_update_non_rtp_src_caps;

	gst_element_class_set_static_metadata(
	    gstelement_class, "Electric Maple RTP Header Extension", GST_RTP_HDREXT_ELEMENT_CLASS,
	    "Extends RTP packets with information for the Electric Maple XR streaming solution.",
	    "Ryan Pavlik <rpavlik@collabora.com>");
	gst_rtp_header_extension_class_set_uri(rtp_hdr_class, GST_RTP_HDREXT_ELECTRIC_MAPLE_URI);
}
