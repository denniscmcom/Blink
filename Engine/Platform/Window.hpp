// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
template <typename Type>
struct Rect;

void create_window();
void destroy_window();
Rect<int> get_window_client_size();
}  // namespace blk
