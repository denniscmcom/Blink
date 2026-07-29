#pragma once

#include "Platform/Log.hpp"

#include <stdlib.h>

#ifdef _DEBUG
/// Triggers a debugger breakpoint if `expression` is false, outputs a message to the debugger and crashes the program.
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
/// Returns the value of `expression`. It triggers a debugger breakpoint if `expression` is false and outputs a message
/// to the debugger.
/// @warning Different behaviour on non-debug targets.
#define BLK_VERIFY(expression) ((expression) ? true : (BLK_LOG("VERIFY FAILED", #expression), BLK_DEBUG_BREAK(), false))
#else
/// Returns the value of `expression`.
#define BLK_VERIFY(expression) (expression)
#endif

#ifdef _DEBUG
/// Returns the value of `expression`. It triggers a debugger breakpoint if `expression` is false, outputs a message
/// to the debugger and crashes the program.
/// @warning Different behaviour on non-debug targets.
#define BLK_ENSURE(expression) BLK_VERIFY(expression)
#else
/// Returns the value of `expression`. It crashes the program if `expression` is `false`.
// TODO: This should call my custom crash reporter (when it's done).
#define BLK_ENSURE(expression) ((expression) ? true : abort())
#endif

#define BLK_ASSUME(expression) __assume(expression)

#ifdef _DEBUG
/// Triggers a debugger breakpoint when reached and outputs a message to the debugger.
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
