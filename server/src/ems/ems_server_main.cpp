// Copyright 2020, Collabora, Ltd.
// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief  Main file for Monado service.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc
 */

#include "xrt/xrt_config_os.h"

#include "util/u_metrics.h"
#include "util/u_logging.h"
#include "util/u_trace_marker.h"

#ifdef XRT_OS_WINDOWS
#include "util/u_windows.h"
#endif

// #include "target_lists.h"


// Insert the on load constructor to init trace marker.
U_TRACE_TARGET_SETUP(U_TRACE_WHICH_SERVICE)


// TODO(chesterton's fence) Shouldn't we just include ipc_server.h here?
extern "C" int
ipc_server_main(int argc, char *argv[]);


int
main(int argc, char *argv[])
{
#ifdef XRT_OS_WINDOWS
	u_win_try_privilege_or_priority_from_args(U_LOGGING_INFO, argc, argv);
#endif

	u_trace_marker_init();
	u_metrics_init();

	int ret = ipc_server_main(argc, argv);

	u_metrics_close();

	return ret;
}
