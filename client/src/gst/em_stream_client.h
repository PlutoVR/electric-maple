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

#define EM_TYPE_STREAM_CLIENT em_stream_client_get_type()

G_DECLARE_FINAL_TYPE(EmStreamClient, em_stream_client, EM, STREAM_CLIENT, GObject)


/*!
 * Create a stream client object, providing the connection object
 *
 * @param connection The connection to use
 *
 * @memberof EmStreamClient
 */
EmStreamClient *
em_stream_client_new(EmConnection *connection);


void
em_stream_client_set_egl_context(EmStreamClient *sc, EGLDisplay display, EGLContext context, EGLSurface pbuffer_surface);

/*!
 * Lock the mutex for the "main" EGL context supplied via @ref em_stream_client_set_egl_context
 */
void
em_stream_client_egl_mutex_lock(EmStreamClient *sc);


/*!
 * Unlock the mutex for the "main" EGL context supplied via @ref em_stream_client_set_egl_context
 */
void
em_stream_client_egl_mutex_unlock(EmStreamClient *sc);
