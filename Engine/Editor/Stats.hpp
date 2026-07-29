#pragma once

namespace blk
{
struct Editor_Context;

void draw_stats(Editor_Context& context);
void draw_fps();
void draw_ms();
void compute_stats(const Editor_Context& context, double delta_time);
}  // namespace blk
