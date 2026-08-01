// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Time.hpp"

#include <chrono>

namespace
{
std::chrono::time_point<std::chrono::steady_clock> timer;
}  // namespace

void
blk::start_global_timer()
{
	timer = std::chrono::steady_clock::now();
}

double
blk::get_global_timer_seconds()
{
	auto current = std::chrono::steady_clock::now();

	return std::chrono::duration<double>(current - timer).count();
}
