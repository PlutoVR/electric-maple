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

#include "pl_comp.h"
#include "pl_driver.h"


#ifdef __cplusplus
extern "C" {
#endif

struct pluto_program
{
	//! Instance base.
	struct xrt_instance xinst_base;

	//! System devices base.
	struct xrt_system_devices xsysd_base;

	// owned by xsysd_base - convenience
	struct xrt_device *head;

	

	//! Space overseer, implemented for now using helper code.
	struct xrt_space_overseer *xso;
};


static inline struct pluto_program *
from_xinst(struct xrt_instance *xinst)
{
	return container_of(xinst, struct pluto_program, xinst_base);
}

static inline struct pluto_program *
from_xsysd(struct xrt_system_devices *xsysd)
{
	return container_of(xsysd, struct pluto_program, xsysd_base);
}

#ifdef __cplusplus
}
#endif
