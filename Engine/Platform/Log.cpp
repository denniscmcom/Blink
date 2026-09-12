// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Log.hpp"

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

namespace
{
blk::Log_Sink sinks[blk::MAX_SINK_COUNT];
uint32_t sink_count = 0;
}  // namespace

blk::Result
blk::create_log_sink(Log_Sink sink)
{
	if (!BLK_VERIFY(sink))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (!BLK_VERIFY(sink_count < MAX_SINK_COUNT))
	{
		return Result::OUT_OF_BOUNDS;
	}

	sinks[sink_count] = sink;
	sink_count += 1;

	return Result::SUCCESS;
}

void
blk::log_msg(const char* tag, const char* filename, int line, const char* fmt, ...)
{
	char msg[MAX_MSG_SIZE];
	int offset = 0;

	if (filename)
	{
		offset = snprintf(msg, MAX_MSG_SIZE, "[ %s ] %s:%i ", tag, filename, line);
	}
	else
	{
		offset = snprintf(msg, MAX_MSG_SIZE, "[ %s ] ", tag);
	}

	if (!BLK_VERIFY(offset >= 0 && offset < MAX_MSG_SIZE))
	{
		return;
	}

	va_list args;
	va_start(args, fmt);

	if (const int written = vsnprintf(msg + offset, MAX_MSG_SIZE - offset, fmt, args); !BLK_VERIFY(written >= 0))
	{
		va_end(args);
		return;
	}

	va_end(args);

	for (uint32_t i = 0; i < sink_count; i++)
	{
		sinks[i](msg);
	}
}
