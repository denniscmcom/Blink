// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Event.hpp"

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"

#include <stdint.h>

/// Maximum events in `Event_Buffer.events`.
constexpr uint32_t MAX_EVENT_COUNT = 1024;

namespace
{
/// Single-producer single-consumer event ring buffer.
///
/// `read_index == write_index` means empty, so the writer must stop one slot short of the reader to keep that
/// comparison unambiguous. Usable capacity is `MAX_EVENT_COUNT - 1`.
struct Event_Buffer
{
	/// Events in the buffer.
	blk::Event events[MAX_EVENT_COUNT];
	/// Position of the next read.
	uint32_t read_index;
	/// Position of the next write.
	uint32_t write_index;
};

Event_Buffer event_buffer = {};
}  // namespace

void
blk::write_event(const Event& event)
{
	if (!BLK_VERIFY(event.type != Event_Type::NONE))
	{
		return;
	}

	const uint32_t next_write_index = (event_buffer.write_index + 1) % MAX_EVENT_COUNT;

	// Full buffer: writing here would make `write_index` reach `read_index` and read as empty. Drop the event.
	if (next_write_index == event_buffer.read_index)
	{
		BLK_WARNING("Event buffer is full, dropping event\n");

		return;
	}

	event_buffer.events[event_buffer.write_index] = event;
	event_buffer.write_index = next_write_index;
}

blk::Event
blk::peek_event()
{
	// Empty buffer: return `Event_Type::NONE`.
	if (event_buffer.read_index == event_buffer.write_index)
	{
		return {};
	}

	return event_buffer.events[event_buffer.read_index];
}

blk::Event
blk::poll_event()
{
	// Empty buffer: return `Event_Type::NONE`.
	if (event_buffer.read_index == event_buffer.write_index)
	{
		return {};
	}

	const Event event = event_buffer.events[event_buffer.read_index];
	event_buffer.read_index = (event_buffer.read_index + 1) % MAX_EVENT_COUNT;

	return event;
}
