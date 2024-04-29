// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief Encapsulate EGL setup/teardown.
 * @author Moshi Turner <moses@collabora.com>
 * @author Rylie Pavlik <rpavlik@collabora.com>
 */
#include "EglData.hpp"

#include "em/em_app_log.h"
#include "em/render/GLError.h"

#include <EGL/egl.h>

#include <stdexcept>

EglData::EglData()
{

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	if (display == EGL_NO_DISPLAY) {
		ALOGE("Failed to get EGL display");
		return;
	}

	bool success = eglInitialize(display, NULL, NULL);

	if (!success) {
		ALOGE("Failed to initialize EGL");
		return;
	}

#define MAX_CONFIGS 1024
	EGLConfig configs[MAX_CONFIGS];

	// RGBA8, multisample not required, ES3, pbuffer and window
	const EGLint attributes[] = {
	    EGL_RED_SIZE,
	    8, //

	    EGL_GREEN_SIZE,
	    8, //

	    EGL_BLUE_SIZE,
	    8, //

	    EGL_ALPHA_SIZE,
	    8, //

	    EGL_SAMPLES,
	    1, //

	    EGL_RENDERABLE_TYPE,
	    EGL_OPENGL_ES3_BIT,

	    EGL_SURFACE_TYPE,
	    (EGL_PBUFFER_BIT | EGL_WINDOW_BIT),

	    EGL_NONE,
	};

	EGLint num_configs = 0;
	CHK_EGL(eglChooseConfig(display, attributes, configs, MAX_CONFIGS, &num_configs));

	if (num_configs == 0) {
		ALOGE("Failed to find suitable EGL config");
		throw std::runtime_error("Failed to find suitable EGL config");
	}
	ALOGI("Got %d egl configs, just taking the first one.", num_configs);

	config = configs[0];

	EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	CHK_EGL(context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes));

	if (context == EGL_NO_CONTEXT) {
		ALOGE("Failed to create EGL context");
		throw std::runtime_error("Failed to create EGL context");
	}
	CHECK_EGL_ERROR();
	ALOGI("EGL: Created context");

	// TODO why are we making a 16x16 pbuffer surface? Do we even need it?
	EGLint surfaceAttributes[] = {
	    EGL_WIDTH,
	    16, //
	    EGL_HEIGHT,
	    16, //
	    EGL_NONE,
	};
	CHK_EGL(surface = eglCreatePbufferSurface(display, config, surfaceAttributes));
	if (surface == EGL_NO_SURFACE) {
		ALOGE("Failed to create EGL surface");
		eglDestroyContext(display, context);
		throw std::runtime_error("Failed to create EGL surface");
	}
	CHECK_EGL_ERROR();
	ALOGI("EGL: Created surface");
}

EglData::~EglData()
{
	EGLDisplay d = display;
	if (d == EGL_NO_DISPLAY) {
		d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	}
	if (surface != EGL_NO_SURFACE) {
		eglDestroySurface(d, surface);
		surface = EGL_NO_SURFACE;
	}

	if (context != EGL_NO_CONTEXT) {
		eglDestroyContext(d, context);
		context = EGL_NO_CONTEXT;
	}
}


void
EglData::makeCurrent() const
{
	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		ALOGE("Failed to make EGL context current");
		CHECK_EGL_ERROR();
		throw std::runtime_error("Could not make EGL context current");
	}
	ALOGI("EGL: Made context current");
}

void
EglData::makeNotCurrent() const
{
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}
