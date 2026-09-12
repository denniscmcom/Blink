// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <stdint.h>

namespace blk
{
/// Controller key.
enum class Key : uint8_t
{
	/// Sentinel value to represent no key.
	/// @warning It should be always at the beginning of the enumeration. Add new keys after this value.
	NONE,

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

	/// Sentinel value to represent the end of the enumeration.
	/// @warning It should be always at the end. Add new keys before this value.
	END,
};

/// Type of the input event.
enum class Event_Type : uint8_t
{
	NONE,
	KEY_DOWN,
	KEY_UP,
	MOUSE_MOVE,
};

/// Input event.
///
/// Specific fields are populated depending on `Event_Type`.
struct Event
{
	Event_Type type;
	Key key;
	int32_t mouse_delta_x;
	int32_t mouse_delta_y;
};

/// Writes an `event` to the event buffer.
void write_event(const Event& event);
/// Gets the next event from the buffer without advancing the read index.
Event peek_event();
/// Gets the next event from the buffer and advances the read index.
Event poll_event();
}  // namespace blk
