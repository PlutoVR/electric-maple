#pragma once

#include <gst/rtp/gstrtphdrext.h>
#include <gst/rtp/rtp.h>


#define GST_TYPE_RTP_ELECTRIC_MAPLE_HDR_EXT (gst_rtp_electric_maple_hdr_ext_get_type())
#define GST_RTP_ELECTRIC_MAPLE_HDR_EXT(obj)                                                                            \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTP_ELECTRIC_MAPLE_HDR_EXT, GstRTPHdrExt))
#define GST_RTP_ELECTRIC_MAPLE_HDR_EXT_CLASS(klass)                                                                    \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RTP_ELECTRIC_MAPLE_HDR_EXT, GstRTPElectricMapleHdrExtClass))
#define GST_IS_RTP_ELECTRIC_MAPLE_HDR_EXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RTP_ELECTRIC_MAPLE_HDR_EXT))
#define GST_IS_RTP_ELECTRIC_MAPLE_HDR_EXT_CLASS(klass)                                                                 \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RTP_ELECTRIC_MAPLE_HDR_EXT))

struct _GstRTPEMHdrExt
{
	GstRTPHeaderExtension payload;

	GstRTPHeaderExtensionFlags supported_flags;
};
