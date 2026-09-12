// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#ifdef _MSC_VER
/// Triggers a debugger breakpoint.
#define BLK_DEBUG_BREAK() __debugbreak()
#else
// TODO (Bug): Compilers other than MSVC never break into the debugger. Every assertion silently continues.

/// No-op.
#define BLK_DEBUG_BREAK() ((void)0)
#endif

namespace blk
{
/// Outputs a message to the debugger.
void log_debugger(const char* msg);
}  // namespace blk
