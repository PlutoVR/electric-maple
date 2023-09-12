// Copyright 2022-2023, PlutoVR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  The EGL utilities of the ElectricMaple XR streaming solution
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */
#include "em_egl.h"

#include "em_app_log.h"

#include <EGL/egl.h>
#include <assert.h>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <type_traits>

void
em_egl_state_save(EmEglState *ees)
{
	ees->context = eglGetCurrentContext();
	ees->read_surface = eglGetCurrentSurface(EGL_READ);
	ees->draw_surface = eglGetCurrentSurface(EGL_DRAW);

	ALOGI("%s : save current draw surface=%p, read surface=%p, context=%p", __FUNCTION__, ees->draw_surface,
	      ees->read_surface, ees->context);
}

void
em_egl_state_restore(const EmEglState *ees, EGLDisplay display)
{
	ALOGI("%s : restore display=%p, draw surface=%p, read surface=%p, context=%p", __FUNCTION__, display,
	      ees->draw_surface, ees->read_surface, ees->context);
	eglMakeCurrent(display, ees->draw_surface, ees->read_surface, ees->context);
}

namespace {
struct EmEglMutex
{
	EmEglMutexIface base;
	std::mutex mutex;
	EmEglState old_state;

	EmEglMutex(EGLDisplay display, EGLContext context);
};
static_assert(std::is_standard_layout<EmEglMutex>::value,
              "Must be standard layout to use the casts required for this interface-implementation style");

bool
egl_mutex_begin(EmEglMutexIface *eemi, EGLSurface draw, EGLSurface read)
{
	auto *eem = reinterpret_cast<EmEglMutex *>(eemi);
	std::unique_lock<std::mutex> lock(eem->mutex);
	em_egl_state_save(&eem->old_state);
	ALOGI("%s : make current display=%p, draw surface=%p, read surface=%p, context=%p", __FUNCTION__,
	      eem->base.display, draw, read, eem->base.context);
	if (eglMakeCurrent(eem->base.display, draw, read, eem->base.context) == EGL_FALSE) {
		ALOGE("%s: Failed make egl context current", __FUNCTION__);
		lock.unlock();
		return false;
	}
	lock.release();
	return true;
}

void
egl_mutex_end(EmEglMutexIface *eemi)
{
	auto *eem = reinterpret_cast<EmEglMutex *>(eemi);
	std::unique_lock<std::mutex> lock(eem->mutex, std::adopt_lock);
	ALOGI("%s: Make egl context un-current", __FUNCTION__);
	em_egl_state_restore(&eem->old_state, eem->base.display);
}

void
egl_mutex_destroy(EmEglMutexIface *eemi)
{
	// adopt into unique_ptr so it will be destroyed
	std::unique_ptr<EmEglMutex> eem(reinterpret_cast<EmEglMutex *>(eemi));
}

inline EmEglMutex::EmEglMutex(EGLDisplay display, EGLContext context)
{
	base.display = display;
	base.context = context;
	base.begin = egl_mutex_begin;
	base.end = egl_mutex_end;
	base.destroy = egl_mutex_destroy;
}

} // namespace

EmEglMutexIface *
em_egl_mutex_create(EGLDisplay display, EGLContext context)
{
	try {
		auto ret = std::make_unique<EmEglMutex>(display, context);

		return &ret.release()->base;
	} catch (std::exception const &e) {
		ALOGE("Caught exception in %s: %s", __FUNCTION__, e.what());
		return NULL;
	} catch (...) {
		ALOGE("Caught exception in %s", __FUNCTION__);
		return NULL;
	}
}
