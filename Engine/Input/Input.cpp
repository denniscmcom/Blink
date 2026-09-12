// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Input/Input.hpp"

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Event.hpp"

#include <stdint.h>
#include <string.h>

void
blk::update_input(Input_State& state)
{
	clear_frame_events(state);

	for (Event event = poll_event(); event.type != Event_Type::NONE; event = poll_event())
	{
		const auto key_index = static_cast<uint8_t>(event.key);

		switch (event.type)
		{
		case Event_Type::NONE:
			// This branch is unreachable.
			break;
		case Event_Type::KEY_DOWN:
			// Only the first down-edge is a press; auto-repeats arrive while already held.
			if (!is_key_held(state, event.key))
			{
				state.key_press_count[key_index] += 1;
			}

			state.held_keys[key_index] = true;
			break;
		case Event_Type::KEY_UP:
			// Only count a release for a key we saw held. A `KEY_UP` without a matching `KEY_DOWN` is not a release.
			if (is_key_held(state, event.key))
			{
				state.key_release_count[key_index] += 1;
			}

			state.held_keys[key_index] = false;
			break;
		case Event_Type::MOUSE_MOVE:
			state.mouse_delta_x += event.mouse_delta_x;
			state.mouse_delta_y += event.mouse_delta_y;
			break;
		}
	}
}

void
blk::clear_frame_events(Input_State& state)
{
	memset(state.key_press_count, 0, sizeof(state.key_press_count));
	memset(state.key_release_count, 0, sizeof(state.key_release_count));

	state.mouse_delta_x = 0;
	state.mouse_delta_y = 0;
}

void
blk::clear_level_events(Input_State& state)
{
	memset(state.held_keys, 0, sizeof(state.held_keys));
}

bool
blk::is_key_held(const Input_State& state, Key key)
{
	BLK_CHECK(static_cast<uint8_t>(key) < MAX_KEY_COUNT);

	return state.held_keys[static_cast<uint8_t>(key)];
}

bool
blk::is_key_press(const Input_State& state, Key key)
{
	BLK_CHECK(static_cast<uint8_t>(key) < MAX_KEY_COUNT);

	return state.key_press_count[static_cast<uint8_t>(key)] > 0;
}

bool
blk::is_key_release(const Input_State& state, Key key)
{
	BLK_CHECK(static_cast<uint8_t>(key) < MAX_KEY_COUNT);

	return state.key_release_count[static_cast<uint8_t>(key)] > 0;
}
