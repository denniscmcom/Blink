// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Debug.hpp"

#include <stdint.h>

#ifndef BLK_SOURCE_PATH_PREFIX_LENGTH
#define BLK_SOURCE_PATH_PREFIX_LENGTH 0
#endif

#define BLK_LOG(tag, ...) blk::log_msg(tag, &__FILE__[BLK_SOURCE_PATH_PREFIX_LENGTH], __LINE__, __VA_ARGS__)

#if defined(_DEBUG) && defined(_TRACE)
#define BLK_TRACE(...) BLK_LOG("TRACE", __VA_ARGS__)
#else
#define BLK_TRACE(...) ((void)0)
#endif

#ifdef _DEBUG
#define BLK_DEBUG(...) BLK_LOG("DEBUG", __VA_ARGS__)
#else
#define BLK_DEBUG(...) ((void)0)
#endif

#define BLK_INFO(...) BLK_LOG("INFO", __VA_ARGS__)

#define BLK_WARNING(...) BLK_LOG("WARNING", __VA_ARGS__)

#ifdef _DEBUG
#define BLK_ERROR(...)                                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		BLK_LOG("ERROR", __VA_ARGS__);                                                                                 \
		/* TODO (WIP): Sometimes it is too much noise. BLK_DEBUG_BREAK(); */                                           \
	} while (0)
#else
#define BLK_ERROR(...) BLK_LOG("ERROR", __VA_ARGS__);
#endif

#ifdef _DEBUG
#define BLK_FATAL(...)                                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		BLK_LOG("FATAL", __VA_ARGS__);                                                                                 \
		BLK_DEBUG_BREAK();                                                                                             \
		abort();                                                                                                       \
	} while (0)
#else
// TODO: This should call my custom crash reporter.
#define BLK_FATAL(...) BLK_LOG("FATAL", __VA_ARGS__);
#endif

namespace blk
{
using Log_Sink = void (*)(const char* msg);

constexpr size_t MAX_MSG_SIZE = 1024;
constexpr uint32_t MAX_SINK_COUNT = 5;

void create_log_sink(Log_Sink sink);
void log_msg(const char* tag, const char* filename, int line, const char* fmt, ...);
}  // namespace blk
