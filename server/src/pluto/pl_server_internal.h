// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 */

#pragma once
#include "xrt/xrt_system.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_instance.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_compositor.h"

#include "util/u_pacing.h"
#include "util/u_logging.h"

// #include "pl_comp.h"
// #include "pl_driver.h"

#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <thread>



struct pluto_program;
struct pluto_hmd;

struct pluto_hmd
{
	struct xrt_device base;

	struct xrt_pose pose;

	// Should outlive us
	struct pluto_program *program;



	enum u_logging_level log_level;
};

struct pluto_program
{
	//! Instance base.
	struct xrt_instance xinst_base;

	//! System devices base.
	struct xrt_system_devices xsysd_base;

	// convenience
	struct pluto_hmd *head;

	//! Space overseer, implemented for now using helper code.
	struct xrt_space_overseer *xso;

};


// compositor interface functions


/*!
 * Creates a @ref pluto_compositor.
 *
 * @ingroup comp_null
 */
xrt_result_t
pluto_compositor_create_system(pluto_program &pp, struct xrt_system_compositor **out_xsysc);


// driver interface functions

struct pluto_hmd *
pluto_hmd_create(pluto_program &pp);

