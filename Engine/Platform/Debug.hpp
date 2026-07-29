#pragma once

#ifdef _MSC_VER
/// Triggers a debugger breakpoint.
#define BLK_DEBUG_BREAK() __debugbreak()
#endif

namespace blk
{
/// Outputs a message to the debugger.
void log_debugger(const char* msg);
}  // namespace blk
