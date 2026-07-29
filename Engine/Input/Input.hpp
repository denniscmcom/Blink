#pragma once

#include "Platform/Event.hpp"

#include <array>
#include <stdint.h>

namespace blk
{
constexpr uint32_t MAX_KEY_COUNT = 256;

struct Input_State
{
	bool held_keys[MAX_KEY_COUNT];
	uint8_t key_press_count[MAX_KEY_COUNT];
	uint8_t key_release_count[MAX_KEY_COUNT];

	int32_t mouse_delta_x;
	int32_t mouse_delta_y;
};

void update_input_state(Input_State& state);
void clear_frame_input_state(Input_State& state);
void clear_level_input_state(Input_State& state);
bool is_key_held(const Input_State& state, Key key);
bool is_key_press(const Input_State& state, Key key);
bool is_key_release(const Input_State& state, Key key);
}  // namespace blk
