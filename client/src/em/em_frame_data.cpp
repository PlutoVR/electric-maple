// Copyright 2022-2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

/*!
 * @file
 * @brief  Frame data collection for the ElectricMaple XR streaming solution
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */

#include "em_frame_data.hpp"
#include "em/em_id_data_accumulator.hpp"
#include "electricmaple.pb.h"
#include <mutex>

namespace em {

void
FrameDataAccumulator::recordDecodeTime(int64_t frameId, int64_t decodeTime)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_accum.addDataFor(frameId, FrameData{decodeTime});
}

void
FrameDataAccumulator::recordDisplayTime(int64_t frameId, int64_t displayTime)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_accum.updateDataFor(frameId, [=](FrameData &data) { data.displayTime = displayTime; });
}

void
FrameDataAccumulator::emitCompleteRecords(PfnEmitUpMessage pfn, void *userdata)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_accum.visitAll([&](id_data_accum::IdType id, FrameData const &data) {
		if (data.decodeTime != 0 && data.displayTime != 0) {
			em_proto_UpMessage message = em_proto_UpMessage_init_default;
			message.has_frame = true;
			message.frame.frame_sequence_id = id;
			message.frame.decode_complete_time = data.decodeTime;
			message.frame.display_time = data.displayTime;

			pfn(&message, userdata);

			return id_data_accum::Command::Drop;
		}
		return id_data_accum::Command::Keep;
	});
}


} // namespace em
