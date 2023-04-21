// Copyright 2020-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup drv_vf
 */

#pragma once

#include "xrt/xrt_frameserver.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup drv_vf Video Fileframeserver driver
 * @ingroup drv
 *
 * @brief Frameserver using a video file.
 */

/*!
 * Create a vf frameserver by opening a video file.
 *
 * @ingroup drv_vf
 */
struct xrt_fs *
vf_fs_open_file(struct xrt_frame_context *xfctx, const char *path);

/*!
 * Create a vf frameserver that uses the videotestsource.
 *
 * @ingroup drv_vf
 */
struct xrt_fs *
vf_fs_videotestsource(struct xrt_frame_context *xfctx, uint32_t width, uint32_t height);


#ifdef __cplusplus
}
#endif
