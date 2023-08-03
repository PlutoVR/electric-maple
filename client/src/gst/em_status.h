// Copyright 2023, PlutoVR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup xrt_fs_em
 */

#pragma once

/// Status of the ElectricMaple remote rendering connection
#include <stdbool.h>
enum em_status
{
	/// Not connected, not connecting, and not waiting before retrying connection.
	EM_STATUS_IDLE_NOT_CONNECTED = 0,
	/// Connecting to the signaling server websocket
	EM_STATUS_CONNECTING,
	/// Connecting to the signaling server websocket (auto retry after error)
	EM_STATUS_CONNECTING_RETRY,
	/// Failed but will retry connection to the signaling server websocket
	EM_STATUS_WILL_RETRY,
	/// Signaling server connection established, negotiating for full WebRTC connection
	// TODO do we need more steps here?
	EM_STATUS_NEGOTIATING,
	/// WebRTC connection established, awaiting data channel
	EM_STATUS_CONNECTED_NO_DATA,
	/// Full WebRTC connection (with data channel) established.
	EM_STATUS_CONNECTED,
	/// Disconnected following a connection error, will not retry.
	EM_STATUS_DISCONNECTED_ERROR,
	/// Disconnected following remote closing of the channel, will not retry.
	EM_STATUS_DISCONNECTED_REMOTE_CLOSE,
};

inline bool
em_status_is_connecting(enum em_status status)
{
	return status == EM_STATUS_CONNECTING || status == EM_STATUS_CONNECTING_RETRY;
}
