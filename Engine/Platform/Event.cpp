#include "Platform/Event.hpp"

#include "Platform/Assert.hpp"

#include <stdint.h>

constexpr uint32_t MAX_EVENT_COUNT = 1024;

namespace
{
/// Single-producer single-consumer event ring buffer.
///
/// `read_index == write_index` means empty, so the writer must stop one slot short of the reader to keep that
/// comparison unambiguous. Usable capacity is `MAX_EVENT_COUNT - 1`.
struct Event_Buffer
{
	blk::Event events[MAX_EVENT_COUNT];
	uint32_t read_index;
	uint32_t write_index;
};

Event_Buffer event_buffer = {};
}  // namespace

void
blk::write_event(const Event& event)
{
	BLK_CHECK(event.type != Event_Type::NONE);

	const uint32_t next_write_index = (event_buffer.write_index + 1) % MAX_EVENT_COUNT;

	// Full buffer: writing here would make `write_index` reach `read_index` and read as empty. Drop the event.
	if (!BLK_VERIFY(next_write_index != event_buffer.read_index))
	{
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
