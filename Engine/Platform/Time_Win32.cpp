// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Time.hpp"

#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"

#include <Windows.h>

namespace blk
{
/// Definition of opaque `Timer`.
struct Timer
{
	/// The allocator used to create the timer.
	Allocator* allocator;
	/// Current value of the high-resolution performance counter.
	LARGE_INTEGER ticks;
};
}  // namespace blk

namespace
{
/// Frequency of the high-resolution performance counter. It's set at system boot, therefore it can be cached.
LARGE_INTEGER counter_frequency;
bool is_counter_frequency_cached = false;
}  // namespace

blk::Result
blk::create_timer(Allocator* allocator, Timer*& timer)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	void* pointer = nullptr;

	if (const Result result = allocate(*allocator, pointer, sizeof(Timer), alignof(Timer)); result != Result::SUCCESS)
	{
		return result;
	}

	timer = static_cast<Timer*>(pointer);
	timer->allocator = allocator;

	if (!is_counter_frequency_cached)
	{
		if (!QueryPerformanceFrequency(&counter_frequency))
		{
			return Result::OS_ERROR;
		}

		is_counter_frequency_cached = true;
	}

	if (!QueryPerformanceCounter(&timer->ticks))
	{
		return Result::OS_ERROR;
	}

	return Result::SUCCESS;
}

void
blk::destroy_timer(Timer* timer)
{
	if (!timer)
	{
		return;
	}

	BLK_CHECK(timer->allocator);
	free(*timer->allocator, timer);
}

blk::Result
blk::get_elapsed_seconds(const Timer* timer, double& elapsed)
{
	BLK_CHECK(is_counter_frequency_cached);
	BLK_CHECK(counter_frequency.QuadPart != 0);

	if (!timer)
	{
		return Result::INVALID_ARGUMENTS;
	}

	LARGE_INTEGER current = {};

	if (!QueryPerformanceCounter(&current))
	{
		return Result::OS_ERROR;
	}

	elapsed =
		static_cast<double>(current.QuadPart - timer->ticks.QuadPart) / static_cast<double>(counter_frequency.QuadPart);

	return Result::SUCCESS;
}
