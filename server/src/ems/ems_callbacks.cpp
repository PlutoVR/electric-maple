// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief  Callback type for incoming data over remote rendering data connection
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup aux_util
 */

#include "ems_callbacks.h"
#include <cstddef>
#include <type_traits>

#include "util/u_generic_callbacks.hpp"

#include <memory>
#include <mutex>
#include <stdint.h>

struct ems_callbacks
{
	std::mutex mutex;
	xrt::auxiliary::util::GenericCallbacks<ems_callbacks_func_t, enum ems_callbacks_event> callbacks_collection;
};

struct ems_callbacks *
ems_callbacks_create()
{
	// auto ret = std::make_unique<ems_callbacks>();

	// return ret.release();
	return new ems_callbacks;
}

void
ems_callbacks_destroy(struct ems_callbacks **ptr_callbacks)
{
	if (!ptr_callbacks) {
		return;
	}
	std::unique_ptr<ems_callbacks> callbacks(*ptr_callbacks);
	// take the lock to wait for anybody else who might be in the lock.
	std::unique_lock<std::mutex> lock(callbacks->mutex);
	lock.unlock();

	*ptr_callbacks = nullptr;
	callbacks.reset();
}

void
ems_callbacks_add(struct ems_callbacks *callbacks, uint32_t event_mask, ems_callbacks_func_t func, void *userdata)
{
	std::unique_lock<std::mutex> lock(callbacks->mutex);
	callbacks->callbacks_collection.addCallback(func, event_mask, userdata);
}

void
ems_callbacks_reset(struct ems_callbacks *callbacks)
{
	std::unique_lock<std::mutex> lock(callbacks->mutex);
	callbacks->callbacks_collection = {};
}

void
ems_callbacks_call(struct ems_callbacks *callbacks, enum ems_callbacks_event event, const em_proto_UpMessage *message)
{
	std::unique_lock<std::mutex> lock(callbacks->mutex);
	auto invoker = [=](enum ems_callbacks_event ev, ems_callbacks_func_t callback, void *userdata) {
		callback(ev, message, userdata);
		return false; // do not remove
	};
	callbacks->callbacks_collection.invokeCallbacks(event, invoker);
}
