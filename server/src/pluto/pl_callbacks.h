// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Callback type for incoming data over remote rendering data connection
 * @author Ryan Pavlik <rpavlik@collabora.com>
 * @ingroup aux_util
 */
#pragma once
#include <stdint.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Event type bitmas
/// @relates pl_callbacks
enum pl_callbacks_event
{
	PL_CALLBACKS_EVENT_TRACKING = 1u << 0u,
	PL_CALLBACKS_EVENT_CONTROLLER = 1u << 1u,
};

/// Callback function type
/// @relates pl_callbacks
typedef void (*pl_callbacks_func_t)(enum pl_callbacks_event, GBytes *bytes, void *userdata);

/// Callbacks data structure
struct pl_callbacks;

/// Allocate a callbacks data structure
/// @public @memberof pl_callbacks
struct pl_callbacks *
pl_callbacks_create();

/// Destroy a callbacks data structure and clear the pointer.
///
/// Does all the null checks for you.
///
/// @public @memberof pl_callbacks
void
pl_callbacks_destroy(struct pl_callbacks **ptr_callbacks);

/// Add a callback to the collection.
///
/// @param callbacks self
/// @param event_mask Bitmask of @ref pl_callbacks_event indicating which events to be called on.
/// @param func Function to call
/// @param userdata Opaque pointer to provide when calling your function
///
/// @public @memberof pl_callbacks
void
pl_callbacks_add(struct pl_callbacks *callbacks, uint32_t event_mask, pl_callbacks_func_t func, void *userdata);

/// Call all callbacks that are interested in @p event
///
/// @param callbacks self
/// @param event The enum @ref pl_callbacks_event describing this event
/// @param bytes The bytes object. We do not add a reference!
///
/// @public @memberof pl_callbacks
void
pl_callbacks_call(struct pl_callbacks *callbacks, enum pl_callbacks_event event, GBytes *bytes);

#ifdef __cplusplus
} // extern "C"
#endif
