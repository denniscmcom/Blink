#pragma once

#include <stdint.h>

namespace blk
{
enum class Key : uint8_t
{
	KEYBOARD_ESC,
	KEYBOARD_F1,
	KEYBOARD_F2,
	KEYBOARD_F3,
	KEYBOARD_Q,
	KEYBOARD_W,
	KEYBOARD_E,
	KEYBOARD_A,
	KEYBOARD_S,
	KEYBOARD_D,
	MOUSE_RIGHT,
};

enum class Event_Type : uint8_t
{
	NONE,
	KEY_DOWN,
	KEY_UP,
	MOUSE_MOVE,
};

struct Event
{
	Event_Type type = Event_Type::NONE;
	Key key;
	uint32_t mouse_delta_x;
	uint32_t mouse_delta_y;
};

void write_event(const Event& event);
Event peek_event();
Event poll_event();
}  // namespace blk
