// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Log.hpp"

#include <stdio.h>

void
blk::log_console(const char* msg)
{
	printf("%s\n", msg);
}
