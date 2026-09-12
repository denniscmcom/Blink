// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Debug.hpp"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef BLK_SOURCE_PATH_PREFIX_LENGTH
/// Offset in bytes to obtain the relative path from the project's root. It is used to truncate the `__FILE__` macro.
///
/// It is defined by `CMake/BlkConfig.cmake`.
#define BLK_SOURCE_PATH_PREFIX_LENGTH 0
#endif

/// Logs a message with a `tag` prefix and call site location.
#define BLK_LOG(tag, ...) blk::log_msg(tag, &__FILE__[BLK_SOURCE_PATH_PREFIX_LENGTH], __LINE__, __VA_ARGS__)

#if defined(_DEBUG) && defined(BLK_TRACE_LEVEL)
/// Logs a trace message with call site location.
///
/// It is only defined if `_DEBUG` and `BLK_TRACE_LEVEL` are defined.
#define BLK_TRACE(...) BLK_LOG("TRACE", __VA_ARGS__)
#else
/// No-op.
///
/// It is only defined if `_DEBUG` or `BLK_TRACE_LEVEL` are not defined.
#define BLK_TRACE(...) ((void)0)
#endif

#ifdef _DEBUG
/// Logs a debug message with call site location.
///
/// It is only defined if `_DEBUG` is defined.
#define BLK_DEBUG(...) BLK_LOG("DEBUG", __VA_ARGS__)
#else
/// No-op.
///
/// It is only defined if `_DEBUG` is not defined.
#define BLK_DEBUG(...) ((void)0)
#endif

/// Logs an info message with call site location.
#define BLK_INFO(...) BLK_LOG("INFO", __VA_ARGS__)

/// Logs a warning message with call site location.
#define BLK_WARNING(...) BLK_LOG("WARNING", __VA_ARGS__)

#ifdef _DEBUG
/// Logs an error message with a call site location, and triggers a debugger breakpoint if `_DEBUG` is defined.
#define BLK_ERROR(...)                                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		BLK_LOG("ERROR", __VA_ARGS__);                                                                                 \
		BLK_DEBUG_BREAK();                                                                                             \
	} while (0)
#else
/// Logs an error message with a call site location.
#define BLK_ERROR(...) BLK_LOG("ERROR", __VA_ARGS__)
#endif

#ifdef _DEBUG
/// Logs a fatal message with a call site location, triggers a debugger breakpoint if `_DEBUG` is defined, and crashes
/// the program.
#define BLK_FATAL(...)                                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		BLK_LOG("FATAL", __VA_ARGS__);                                                                                 \
		BLK_DEBUG_BREAK();                                                                                             \
		abort();                                                                                                       \
	} while (0)
#else
// TODO: This should call my custom crash reporter.

/// Logs a fatal message with a call site location, send the crash information to the crash reporter, and crashes the
/// program.
#define BLK_FATAL(...) BLK_NOT_IMPLEMENTED()
#endif

namespace blk
{
enum class Result;

/// Function signature to implement a log sink.
using Log_Sink = void (*)(const char* msg);

/// Maximum size of a message.
constexpr size_t MAX_MSG_SIZE = 1024;
/// Maximum active sinks.
constexpr uint32_t MAX_SINK_COUNT = 5;

/// Creates a log sink.
Result create_log_sink(Log_Sink sink);
/// Logs a message prefixed with `tag`, `filename` and `line`.
void log_msg(const char* tag, const char* filename, int line, const char* fmt, ...);
}  // namespace blk
