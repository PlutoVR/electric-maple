// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief  Callback type for incoming data over remote rendering data connection
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup aux_util
 */
#pragma once
#include <stdint.h>
#include <glib.h>

typedef struct _em_proto_UpMessage em_proto_UpMessage;


#ifdef __cplusplus
extern "C" {
#endif

/// Event type bitmas
/// @relates ems_callbacks
enum ems_callbacks_event
{
	EMS_CALLBACKS_EVENT_TRACKING = 1u << 0u,
	EMS_CALLBACKS_EVENT_CONTROLLER = 1u << 1u,
};

/// Callback function type
/// @relates ems_callbacks
typedef void (*ems_callbacks_func_t)(enum ems_callbacks_event, const em_proto_UpMessage *message, void *userdata);

/// Callbacks data structure
struct ems_callbacks;

/// Allocate a callbacks data structure
/// @public @memberof ems_callbacks
struct ems_callbacks *
ems_callbacks_create();

/// Destroy a callbacks data structure and clear the pointer.
///
/// Does all the null checks for you.
///
/// @public @memberof ems_callbacks
void
ems_callbacks_destroy(struct ems_callbacks **ptr_callbacks);

/// Add a callback to the collection.
///
/// @param callbacks self
/// @param event_mask Bitmask of @ref ems_callbacks_event indicating which events to be called on.
/// @param func Function to call
/// @param userdata Opaque pointer to provide when calling your function
///
/// @public @memberof ems_callbacks
void
ems_callbacks_add(struct ems_callbacks *callbacks, uint32_t event_mask, ems_callbacks_func_t func, void *userdata);

/// Call all callbacks that are interested in @p event
///
/// @param callbacks self
/// @param event The enum @ref ems_callbacks_event describing this event
/// @param message The decoded message. We pass yours, we do not copy it!
///
/// @public @memberof ems_callbacks
void
ems_callbacks_call(struct ems_callbacks *callbacks, enum ems_callbacks_event event, const em_proto_UpMessage *message);

/// Clear all callbacks.
///
/// For use prior to starting to destroy things that may have registered callbacks.
///
/// @param callbacks self
///
/// @public @memberof ems_callbacks
void
ems_callbacks_reset(struct ems_callbacks *callbacks);

#ifdef __cplusplus
} // extern "C"
#endif
