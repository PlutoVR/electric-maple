// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for GST part of ElectricMaple XR streaming frameserver
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @ingroup xrt_fs_em
 */
#pragma once

#include <gst/gstmessage.h>

void
em_gst_message_debug(const char *function, GstMessage *msg);

#define LOG_MSG(MSG)                                                                                                   \
	do {                                                                                                           \
		em_gst_message_debug(__FUNCTION__, MSG);                                                               \
	} while (0)
