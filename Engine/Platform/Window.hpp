#pragma once

namespace blk
{
template <typename Type>
struct Rect;

void create_window();
void destroy_window();
Rect<int> get_window_client_size();
}  // namespace blk
