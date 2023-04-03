// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Shared default implementation of the instance with compositor.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include "xrt/xrt_system.h"
#include "xrt/xrt_instance.h"
#include "xrt/xrt_config_drivers.h"

#include "util/u_misc.h"
#include "util/u_builders.h"
#include "util/u_trace_marker.h"

#include "pl_server_internal.h"
#include "../../drivers/pl_driver.h"
#include <assert.h>

#include "main/comp_main_interface.h"

extern "C" {

/*
 *
 * System devices functions.
 *
 */

static void
pluto_system_devices_destroy(struct xrt_system_devices *xsysd)
{
	struct pluto_program *sp = from_xsysd(xsysd);

	for (size_t i = 0; i < xsysd->xdev_count; i++) {
		xrt_device_destroy(&xsysd->xdevs[i]);
	}

	(void)sp; // We are apart of pluto_program, do not free.
}



/*
 *
 * Instance functions.
 *
 */

static xrt_result_t
pluto_instance_get_prober(struct xrt_instance *xinst, struct xrt_prober **out_xp)
{
	return XRT_ERROR_PROBER_NOT_SUPPORTED;
}

// This is where
static xrt_result_t
pluto_instance_create_system(struct xrt_instance *xinst,
                             struct xrt_system_devices **out_xsysd,
                             struct xrt_space_overseer **out_xso,
                             struct xrt_system_compositor **out_xsysc)
{
	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);
	assert(out_xso != NULL);
	assert(*out_xso == NULL);
	assert(out_xsysc == NULL || *out_xsysc == NULL);

	struct xrt_system_compositor *xsysc = NULL;
	struct xrt_space_overseer *xso = NULL;

	xrt_result_t xret = XRT_SUCCESS;


	struct pluto_program *sp = from_xinst(xinst);

	*out_xsysd = &sp->xsysd_base;
	*out_xso = sp->xso;

	// Early out if we only want devices.
	if (out_xsysc == NULL) {
		return XRT_SUCCESS;
	}

	//! @todo We're creating the regular compositor here, which is good for testing, but we need to make a
	// Pluto-compositor :)

	if (xret == XRT_SUCCESS && xsysc == NULL) {
		xret = comp_main_create_system_compositor(sp->xsysd_base.roles.head, NULL, &xsysc);
	}

	*out_xsysc = xsysc;

	return XRT_SUCCESS;
}

static void
pluto_instance_destroy(struct xrt_instance *xinst)
{
	struct pluto_program *sp = from_xinst(xinst);

	delete sp;
}


/*
 *
 * Exported function(s).
 *
 */

void
pluto_system_devices_init(struct pluto_program *sp)
{
	sp->xsysd_base.destroy = pluto_system_devices_destroy;


	struct xrt_device *head = pluto_hmd_create();

	// Setup the device base as the only device.
	sp->xsysd_base.xdevs[0] = head;
	sp->xsysd_base.xdev_count = 1;
	sp->xsysd_base.roles.head = head;

	u_builder_create_space_overseer(&sp->xsysd_base, &sp->xso);
}

void
pluto_instance_init(struct pluto_program *sp)
{
	sp->xinst_base.create_system = pluto_instance_create_system;
	sp->xinst_base.get_prober = pluto_instance_get_prober;
	sp->xinst_base.destroy = pluto_instance_destroy;
}

xrt_result_t
xrt_instance_create(struct xrt_instance_info *ii, struct xrt_instance **out_xinst)
{
	u_trace_marker_init();

	struct pluto_program *sp = new pluto_program();

	pluto_system_devices_init(sp);
	pluto_instance_init(sp);

	*out_xinst = &sp->xinst_base;

	return XRT_SUCCESS;
}
} // extern "C"