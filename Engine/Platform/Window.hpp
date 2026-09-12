// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <stddef.h>

namespace blk
{
struct Allocator;
enum class Result;

template <typename Type>
struct Rect;

/// Maximum size of the window title.
constexpr size_t MAX_WINDOW_TITLE_SIZE = 1024;

/// Creates the main window of the engine.
///
/// Called by `Launcher/`.
Result create_window(Allocator* allocator, const char* title);
/// Destroys the main window of the engine.
///
/// Called by `Launcher/`.
void destroy_window();
/// Gets the window client rectangle size. The client size is the canvas that the caller can write to.
Result get_window_client_rect(Rect<unsigned>& rect);
}  // namespace blk
