// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Pluto HMD device
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Ryan Pavlik <ryan.pavlik@collabora.com>
 * @author Moshi Turner <moses@collabora.com>
 * @ingroup drv_sample
 */

#pragma once
#include "xrt/xrt_device.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_device *
pluto_hmd_create(void);


#ifdef __cplusplus
}
#endif
