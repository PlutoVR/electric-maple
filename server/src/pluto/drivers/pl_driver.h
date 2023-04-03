// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief Pluto HMD device
 *
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
