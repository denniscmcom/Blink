#pragma once

#include <imgui.h>

namespace blk
{
template <typename Type>
struct Rect;

ImVec2 to_imvec2(const Rect<float>& rect);
Rect<float> to_rect2(const ImVec2& vec);
}  // namespace blk
