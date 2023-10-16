// Copyright 2019-2023, Collabora, Ltd.
// Copyright 2023, PlutoVR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for remote rendering compositor.
 *
 * Based on the null compositor
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup comp_pl
 */

#pragma once

#include "xrt/xrt_gfx_vk.h"
#include "xrt/xrt_instance.h"

#include "os/os_time.h"

#include "util/u_threading.h"
#include "util/u_logging.h"
#include "util/u_pacing.h"
#include "util/u_var.h"
#include "util/u_sink.h"

#include "util/comp_base.h"

#include "gstreamer/gst_pipeline.h"
#include "gstreamer/gst_sink.h"
#include "gst/ems_gstreamer_pipeline.h"


#include "pl_server_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup comp_pl Remote rendering compositor
 * @ingroup xrt
 * @brief A small compositor that outputs to a video encoding/streaming pipeline.
 */


/*
 *
 * Structs, enums and defines.
 *
 */

/*!
 * State to emulate state transitions correctly.
 *
 * @ingroup comp_pl
 */
enum pl_comp_state
{
	PLUTO_COMP_COMP_STATE_UNINITIALIZED = 0,
	PLUTO_COMP_COMP_STATE_READY = 1,
	PLUTO_COMP_COMP_STATE_PREPARED = 2,
	PLUTO_COMP_COMP_STATE_VISIBLE = 3,
	PLUTO_COMP_COMP_STATE_FOCUSED = 4,
};

/*!
 * Tracking frame state.
 *
 * @ingroup comp_pl
 */
struct pl_comp_frame
{
	int64_t id;
	uint64_t predicted_display_time_ns;
	uint64_t desired_present_time_ns;
	uint64_t present_slop_ns;
};

/*!
 * Main compositor struct tying everything in the compositor together.
 *
 * @implements xrt_compositor_native, comp_base.
 * @ingroup comp_pl
 */
struct pluto_compositor
{
	struct comp_base base;

	// This thing should outlive us
	struct pluto_program *program;

	//! The device we are displaying to.
	struct xrt_device *xdev;

	//! Pacing helper to drive us forward.
	struct u_pacing_compositor *upc;

	struct
	{
		enum u_logging_level log_level;

		//! Frame interval that we are using.
		uint64_t frame_interval_ns;
	} settings;

	// Kept here for convenience.
	struct xrt_system_compositor_info sys_info;

	//! State for generating the correct set of events.
	enum pl_comp_state state;

	//! @todo Insert your own required members here

	struct
	{
		struct pl_comp_frame waited;
		struct pl_comp_frame rendering;
	} frame;


	struct xrt_frame_context xfctx = {};

	struct vk_cmd_pool cmd_pool = {};

	struct vk_image_readback_to_xf_pool *pool = nullptr;
	int image_sequence;
	struct u_sink_debug debug_sink;

	struct
	{
		VkDeviceMemory device_memory;
		VkImage image;
	} bounce;

	bool pipeline_playing = false;
	struct gstreamer_pipeline *gstreamer_pipeline;
	struct gstreamer_sink *hackers_gstreamer_sink;
	struct xrt_frame_sink *hackers_xfs;

	uint64_t offset_ns;
};


/*
 *
 * Functions and helpers.
 *
 */

/*!
 * Convenience function to convert a xrt_compositor to a pluto_compositor.
 * (Down-cast helper.)
 *
 * @private @memberof pluto_compositor
 * @ingroup comp_pl
 */
static inline struct pluto_compositor *
pluto_compositor(struct xrt_compositor *xc)
{
	return (struct pluto_compositor *)xc;
}

/*!
 * Spew level logging.
 *
 * @relates pluto_compositor
 * @ingroup comp_pl
 */
#define PLUTO_COMP_TRACE(c, ...) U_LOG_IFL_T(c->settings.log_level, __VA_ARGS__);

/*!
 * Debug level logging.
 *
 * @relates pluto_compositor
 */
#define PLUTO_COMP_DEBUG(c, ...) U_LOG_IFL_D(c->settings.log_level, __VA_ARGS__);

/*!
 * Info level logging.
 *
 * @relates pluto_compositor
 * @ingroup comp_pl
 */
#define PLUTO_COMP_INFO(c, ...) U_LOG_IFL_I(c->settings.log_level, __VA_ARGS__);

/*!
 * Warn level logging.
 *
 * @relates pluto_compositor
 * @ingroup comp_pl
 */
#define PLUTO_COMP_WARN(c, ...) U_LOG_IFL_W(c->settings.log_level, __VA_ARGS__);

/*!
 * Error level logging.
 *
 * @relates pluto_compositor
 * @ingroup comp_pl
 */
#define PLUTO_COMP_ERROR(c, ...) U_LOG_IFL_E(c->settings.log_level, __VA_ARGS__);


#ifdef __cplusplus
}
#endif
