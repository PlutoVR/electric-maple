// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief Encapsulate EGL setup/teardown.
 * @author Rylie Pavlik <rpavlik@collabora.com>
 */

#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>

struct EglData
{
	/// Creates an R8G8B8A8 ES3 context and pbuffer surface (for some reason)
	EglData();

	/// Calls reset
	~EglData();

	// do not move
	EglData(const EglData &) = delete;
	// do not move
	EglData(EglData &&) = delete;

	// do not copy
	EglData &
	operator=(const EglData &) = delete;
	// do not copy
	EglData &
	operator=(EglData &&) = delete;


	bool
	isReady() const noexcept
	{
		return display != EGL_NO_DISPLAY && context != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE;
	}

	void
	makeCurrent() const;

	void
	makeNotCurrent() const;

	EGLDisplay display = EGL_NO_DISPLAY;
	EGLContext context = EGL_NO_CONTEXT;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLConfig config = nullptr;
};
