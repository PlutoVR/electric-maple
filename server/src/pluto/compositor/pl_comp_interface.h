// Copyright 2019-2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for null compositor interfaces.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_null
 */


#pragma once

#include "xrt/xrt_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif


struct xrt_device;
struct xrt_system_compositor;

/*!
 * Creates a @ref null_compositor.
 *
 * @ingroup comp_null
 */
xrt_result_t
null_compositor_create_system(struct xrt_device *xdev, struct xrt_system_compositor **out_xsysc);


#ifdef __cplusplus
}
#endif
