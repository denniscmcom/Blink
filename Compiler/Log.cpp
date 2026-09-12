// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Log.hpp"

#include <stdio.h>

void
blk::log_console(const char* msg)
{
	printf("%s", msg);

	// Stdout is fully buffered when redirected to a pipe –e.g. the build system-, and `BLK_DEBUG_BREAK` kills the
	// process before the buffer is flushed.
	fflush(stdout);
}
