// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: MIT
/*!
 * @file
 * @brief  Includes required by openxr_platform.h
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#ifdef __ANDROID__
#include <jni.h>
#include <EGL/egl.h>
#endif

#include <GLES3/gl3.h>


#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
