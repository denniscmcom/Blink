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
/// Win32 `Application`.
struct Application
{
	/// Win32 `HINSTANCE`.
	HINSTANCE hinstance;
	/// Application should quit as soon as possible.
	bool should_quit;
	/// The OS cursor is hidden.
	bool is_cursor_hidden;
	/// Allocator used by application resources.
	Allocator* allocator;
};
#endif

Application* get_application();
}  // namespace blk
