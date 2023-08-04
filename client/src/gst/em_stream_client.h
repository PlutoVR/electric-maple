// Copyright 2022-2023, PlutoVR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for the stream client module of the ElectricMaple XR streaming solution
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */
#pragma once

#include "em_connection.h"

#include <EGL/egl.h>
#include <glib-object.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


struct em_sample;


#define EM_TYPE_STREAM_CLIENT em_stream_client_get_type()

G_DECLARE_FINAL_TYPE(EmStreamClient, em_stream_client, EM, STREAM_CLIENT, GObject)


/*!
 * Create a stream client object, providing the connection object
 *
 * @memberof EmStreamClient
 */
EmStreamClient *
em_stream_client_new();

/*!
 * Initialize the EGL context and surface.
 *
 * Must be called from a thread where it is safe to make the provided context active.
 * After calling this method, **do not** manually make this context active again: instead use
 * @ref em_stream_client_egl_begin and @ref em_stream_client_egl_end
 *
 * @param sc self
 * @param display EGL display
 * @param context An EGL context created for use in the Android main loop.
 * @param pbuffer_surface An EGL pbuffer surface created for the @p context
 * TODO not sure what the surface is actually used for...
 */
void
em_stream_client_set_egl_context(EmStreamClient *sc,
                                 EGLDisplay display,
                                 EGLContext context,
                                 EGLSurface pbuffer_surface);

/*!
 * Lock the mutex for the "main" EGL context supplied via @ref em_stream_client_set_egl_context
 *
 * Typically you will want to use @ref em_stream_client_egl_begin instead.
 */
void
em_stream_client_egl_mutex_lock(EmStreamClient *sc);


/*!
 * Unlock the mutex for the "main" EGL context supplied via @ref em_stream_client_set_egl_context
 *
 * Typically you will want to use @ref em_stream_client_egl_end instead.
 */
void
em_stream_client_egl_mutex_unlock(EmStreamClient *sc);

/*!
 * Lock the mutex for the "main" EGL context supplied via @ref em_stream_client_set_egl_context and set it as current,
 * with your choice of EGL surfaces.
 *
 * @return true if successful - you will need to call @ref em_stream_client_egl_end when done using EGL/GL/GLES to
 * restore previous context/surfaces and unlock.
 */
bool
em_stream_client_egl_begin(EmStreamClient *sc, EGLSurface draw, EGLSurface read);

/*!
 * Lock the mutex for the "main" EGL context supplied via @ref em_stream_client_set_egl_context and set it as current,
 * using the pbuffer surface supplied to that same function.
 *
 * Works just like @ref em_stream_client_egl_begin except it uses the surface you already told us about.
 *
 * @return true if successful - you will need to call @ref em_stream_client_egl_end when done using EGL/GL/GLES to
 * restore previous context/surfaces and unlock.
 */
bool
em_stream_client_egl_begin_pbuffer(EmStreamClient *sc);

/*!
 * Restore previous EGL context and surfaces, and unlock the mutex for the "main" EGL context supplied via @ref
 * em_stream_client_set_egl_context
 */
void
em_stream_client_egl_end(EmStreamClient *sc);

/*!
 * Start the GMainLoop embedded in this object in a new thread
 *
 * @param connection The connection to use
 */
void
em_stream_client_spawn_thread(EmStreamClient *sc, EmConnection *connection);

/*!
 * Stop the pipeline and the mainloop thread.
 */
void
em_stream_client_stop(EmStreamClient *sc);

/*!
 * Attempt to retrieve a sample, if one has been decoded.
 *
 * Non-null return values need to be released with @ref em_stream_client_release_sample
 */
struct em_sample *
em_stream_client_try_pull_sample(EmStreamClient *sc);

/*!
 * Release a sample returned from @ref em_stream_client_try_pull_sample
 */
void
em_stream_client_release_sample(EmStreamClient *sc, struct em_sample *ems);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
