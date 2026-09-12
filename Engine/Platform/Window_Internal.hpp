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
struct Allocator;

#ifdef _WIN32
/// Win32 window.
struct Window
{
	/// `Allocator` used to create the window.
	Allocator* allocator;
	HWND hwnd;
};
#endif

/// Returns a handle to the win32 window.
Window* get_window();
}  // namespace blk
