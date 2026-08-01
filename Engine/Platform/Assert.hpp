// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Debug.hpp"
#include "Engine/Platform/Log.hpp"

#include <stdlib.h>

#ifdef _DEBUG
/// Triggers a debugger breakpoint if `expression` is false, logs a message and crashes the program.
/// @warning Different behaviour on non-debug targets.
#define BLK_CHECK(expression)                                                                                          \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(expression))                                                                                             \
		{                                                                                                              \
			BLK_LOG("CHECK FAILED", #expression);                                                                      \
			BLK_DEBUG_BREAK();                                                                                         \
			abort();                                                                                                   \
		}                                                                                                              \
	} while (0)
#else
/// No-op in non-debug targets.
#define BLK_CHECK(expression) ((void)0)
#endif

#ifdef _DEBUG
/// Triggers a debugger breakpoint if `expression` is false, logs a message and crashes the program.
/// @warning Different behaviour on non-debug targets.
#define BLK_ENSURE(expression)                                                                                         \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(expression))                                                                                             \
		{                                                                                                              \
			BLK_LOG("ENSURE FAILED", #expression);                                                                     \
			BLK_DEBUG_BREAK();                                                                                         \
			abort();                                                                                                   \
		}                                                                                                              \
	} while (0)
#else
/// Reports an error and crashes the program if `expression` is false.
// TODO: This should call my custom crash reporter (when it's done).
#define BLK_ENSURE(expression)                                                                                         \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(expression))                                                                                             \
		{                                                                                                              \
			BLK_LOG("ENSURE FAILED", #expression);                                                                     \
			abort();                                                                                                   \
		}                                                                                                              \
	} while (0)
#endif

#ifdef _DEBUG
/// Returns the value of `expression`. It triggers a debugger breakpoint if `expression` is false and logs a message.
/// @warning Different behaviour on non-debug targets.
#define BLK_VERIFY(expression) ((expression) ? true : (BLK_LOG("VERIFY FAILED", #expression), BLK_DEBUG_BREAK(), false))
#else
/// Returns the value of `expression`. Logs a message if `expression` is false.
#define BLK_VERIFY(expression) ((expression) ? true : (BLK_LOG("VERIFY FAILED", #expression), false))
#endif

#define BLK_ASSUME(expression) __assume(expression)

#ifdef _DEBUG
/// Triggers a debugger breakpoint when reached and logs a message.
/// @warning Different behaviour on non-debug targets.
#define BLK_NOT_IMPLEMENTED()                                                                                          \
	do                                                                                                                 \
	{                                                                                                                  \
		BLK_LOG("NOT IMPLEMENTED", "\n");                                                                              \
		BLK_DEBUG_BREAK();                                                                                             \
	} while (0)
#else
/// Fails compilation.
#define BLK_NOT_IMPLEMENTED() ((void)sizeof(char[-1]))
#endif
