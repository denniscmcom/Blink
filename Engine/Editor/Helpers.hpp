// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <imgui.h>

namespace blk
{
template <typename Type>
struct Rect;

ImVec2 to_imvec2(const Rect<float>& rect);
Rect<float> to_rect2(const ImVec2& vec);
}  // namespace blk
