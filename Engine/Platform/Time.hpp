// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Allocator;
enum class Result;

/// Opaque handle to a timer.
struct Timer;

/// Creates timer.
Result create_timer(Allocator* allocator, Timer*& timer);
/// Destroys timer.
void destroy_timer(Timer* timer);
/// Writes the elapsed seconds since `timer` was created to `elapsed`.
Result get_elapsed_seconds(const Timer* timer, double& elapsed);
}  // namespace blk
