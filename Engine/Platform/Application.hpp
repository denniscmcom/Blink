// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#ifdef _WIN32
#include <Windows.h>

/// Main entry function.
/// @note Handled by `Launcher/`.
#define BLK_ENTRY()                                                                                                    \
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, const int nShowCmd)
#else
// TODO (Bug): `BLK_ENTRY` is not defined for other platforms, so `Launcher/` does not build there.
#endif

namespace blk
{
struct Allocator;
enum class Result;
template <typename Type>
struct Rect;

#ifdef _WIN32
/// Creates the application.
/// @note Handled by `Launcher/`.
Result create_application(Allocator* allocator, HINSTANCE hinstance);
#else
// TODO (Bug): `create_application` is not declared for other platforms, so `Launcher/` does not build there.
#endif

/// Destroys the application.
/// @note Called from `Launcher/`.
void destroy_application();
/// Checks if the application is currently running.
bool is_application_running();
/// Shows the OS cursor.
void show_cursor();
/// Hides the OS cursor.
void hide_cursor();
/// Moves the OS cursor to `position`.
Result set_cursor_position(const Rect<int>& position);
/// Writes the OS cursor position to `position`.
Result get_cursor_position(Rect<int>& position);
}  // namespace blk
