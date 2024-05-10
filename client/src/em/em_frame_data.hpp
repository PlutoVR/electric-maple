// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief  Internal header for a data structure used by the ElectricMaple XR streaming solution
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#pragma once

#include "em/em_id_data_accumulator.hpp"

#include <glib-object.h>

#include <cstddef>
#include <cstdint>
#include <mutex>

typedef struct _em_proto_UpMessage em_proto_UpMessage;

namespace em {

using PfnEmitUpMessage = void (*)(em_proto_UpMessage *, void *);

struct FrameData
{
	int64_t decodeTime;
	int64_t displayTime;
};

class FrameDataAccumulator
{
public:
	FrameDataAccumulator() = default;

	void
	recordDecodeTime(int64_t frameId, int64_t decodeTime);

	void
	recordDisplayTime(int64_t frameId, int64_t displayTime);

	void
	emitCompleteRecords(PfnEmitUpMessage pfn, void *userdata);

private:
	/// basically arbitrary
	static constexpr std::size_t kMaxFrameData = 5;
	IdDataAccumulator<FrameData, kMaxFrameData> m_accum;
	std::mutex m_mutex;
};
} // namespace em
