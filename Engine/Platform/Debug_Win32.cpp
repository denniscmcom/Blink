// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Debug.hpp"

#include <Windows.h>

void
blk::log_debugger(const char* msg)
{
	OutputDebugStringA(msg);
}
