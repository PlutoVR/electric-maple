// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Callback type for incoming data over remote rendering data connection
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup aux_util
 */

#include "pl_callbacks.h"
#include <cstddef>
#include <type_traits>

#include "util/u_generic_callbacks.hpp"


#include <memory>
#include <stdint.h>

struct pl_callbacks
{
	xrt::auxiliary::util::GenericCallbacks<pl_callbacks_func_t, enum pl_callbacks_event> callbacks_collection;
};

struct pl_callbacks *
pl_callbacks_create()
{
	// auto ret = std::make_unique<pl_callbacks>();

	// return ret.release();
	return new pl_callbacks;
}

void
pl_callbacks_destroy(struct pl_callbacks **ptr_callbacks)
{
	if (!ptr_callbacks) {
		return;
	}
	std::unique_ptr<pl_callbacks> callbacks(*ptr_callbacks);
	*ptr_callbacks = nullptr;
	callbacks.reset();
}

void
pl_callbacks_add(struct pl_callbacks *callbacks, uint32_t event_mask, pl_callbacks_func_t func, void *userdata)
{
	callbacks->callbacks_collection.addCallback(func, event_mask, userdata);
}

void
pl_callbacks_reset(struct pl_callbacks *callbacks) {
	callbacks->callbacks_collection = {};
}

void
pl_callbacks_call(struct pl_callbacks *callbacks, enum pl_callbacks_event event, GBytes *bytes)
{
	auto invoker = [=](enum pl_callbacks_event ev, pl_callbacks_func_t callback, void *userdata) {
		callback(ev, bytes, userdata);
		return false; // do not remove
	};
	callbacks->callbacks_collection.invokeCallbacks(event, invoker);
}

// auto invoker = [](MyEvent event, callback_t callback, void *userdata) { return callback(event, userdata); };
