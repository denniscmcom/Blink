// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

namespace blk
{
#ifdef _WIN32
struct Application
{
	HINSTANCE hinstance;
	bool should_quit;
	bool is_cursor_hidden;
};
#endif

Application* get_application();
}  // namespace blk
