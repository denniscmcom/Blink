// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Event.hpp"

#include <stdint.h>

namespace blk
{
/// Maximum number of keys supported.
constexpr auto MAX_KEY_COUNT = static_cast<uint8_t>(Key::END);

/// State of frame-type and level-type events.
///
/// Level-type events are shared between frames.
/// Frame-type events are cleared at the start of a new frame.
struct Input_State
{
	/// Array of keys held down indexed by `static_cast<uint8_t>(Key)`.
	///
	/// Level-type event.
	bool held_keys[MAX_KEY_COUNT];
	/// Array of key press counts indexed by `static_cast<uint8_t>(Key)`.
	///
	/// Frame-type event.
	uint8_t key_press_count[MAX_KEY_COUNT];
	/// Array of key release counts indexed by `static_cast<uint8_t>(Key)`.
	///
	/// Frame-type event.
	uint8_t key_release_count[MAX_KEY_COUNT];
	/// Mouse delta movement in the `x` axis.
	///
	/// Frame-type event.
	int32_t mouse_delta_x;
	/// Mouse delta movement in the `y` axis.
	///
	/// Frame-type event.
	int32_t mouse_delta_y;
};

/// Updates `state` with new user input. It is called by `Launcher/` once per frame.
void update_input(Input_State& state);
/// Clears the frame-type events.
void clear_frame_events(Input_State& state);
// TODO (Bug): `clear_level_events` is never called, so `held_keys` survives a focus loss. Holding a key and switching
// window leaves it held forever, because the matching `KEY_UP` is delivered to the other window. It needs an
// `Event_Type::FOCUS_LOST` emitted from `WM_ACTIVATEAPP` in `Window_Win32.cpp` and handled in `update_input`.

/// Clears the level-type events.
void clear_level_events(Input_State& state);
/// Checks if `key` is held.
bool is_key_held(const Input_State& state, Key key);
/// Checks if `key` is pressed.
bool is_key_press(const Input_State& state, Key key);
/// Checks if `key` is released.
bool is_key_release(const Input_State& state, Key key);
}  // namespace blk
