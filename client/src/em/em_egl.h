// Copyright 2022-2023, PlutoVR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for the EGL utilities of the ElectricMaple XR streaming solution
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */
#pragma once

#include <EGL/egl.h>

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct EmEglState
{
	EGLContext context;
	EGLSurface read_surface;
	EGLSurface draw_surface;
} EmEglState;

/*!
 * Save the current EGL context, read surface, and draw surface to restore later.
 */
void
em_egl_state_save(EmEglState *ees);

/*!
 * Load the current EGL context, read surface, and draw surface from a structure.
 */
void
em_egl_state_restore(const EmEglState *ees, EGLDisplay display);

struct EmEglMutexIface;

/*!
 * Interface for controlling access to an EGL context.
 */
typedef struct EmEglMutexIface
{
	EGLDisplay display;

	EGLContext context;

	/*!
	 * Lock the mutex for this EGL context and set it as current, with your choice of EGL surfaces.
	 *
	 * @return true if successful - you will need to call @ref em_egl_mutex_end when done using EGL/GL/GLES to
	 * restore previous context/surfaces and unlock.
	 */
	bool (*begin)(struct EmEglMutexIface *eem, EGLSurface draw, EGLSurface read);

	/*!
	 * Restore previous EGL context and surfaces, and unlock the mutex for the "main" EGL context supplied via @ref
	 * em_stream_client_set_egl_context
	 */
	void (*end)(struct EmEglMutexIface *eem);

	/*!
	 * Free this structure.
	 */
	void (*destroy)(struct EmEglMutexIface *eem);
} EmEglMutexIface;

/*!
 * Lock the mutex for the "main" EGL context supplied via @ref em_stream_client_set_egl_context and set it as current,
 * with your choice of EGL surfaces.
 *
 * @return true if successful - you will need to call @ref em_egl_mutex_end when done using EGL/GL/GLES to
 * restore previous context/surfaces and unlock.
 */
static inline bool
em_egl_mutex_begin(EmEglMutexIface *eemi, EGLSurface draw, EGLSurface read)
{
	return (eemi->begin)(eemi, draw, read);
}

/*!
 * Restore previous EGL context and surfaces, and unlock the mutex for the "main" EGL context supplied via @ref
 * em_stream_client_set_egl_context
 */
static inline void
em_egl_mutex_end(EmEglMutexIface *eemi)
{
	(eemi->end)(eemi);
}

/*!
 * Free the implementation of this interface stored in the pointed-to variable, checking for null and setting to null
 * after destruction.
 */
static inline void
em_egl_mutex_destroy(EmEglMutexIface **ptr_eemi)
{
	if (!ptr_eemi) {
		return;
	}
	EmEglMutexIface *eemi = *ptr_eemi;
	if (!eemi) {
		return;
	}
	(eemi->destroy)(eemi);
	*ptr_eemi = NULL;
}

/*!
 * Create a default implementation of the @ref EmEglMutexIface interface.
 */
EmEglMutexIface *
em_egl_mutex_create(EGLDisplay display, EGLContext context);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
