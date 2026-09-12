// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{

// TODO (Consistency): `Rect` is not a rectangle, it is a pair of components used for three different things.
// A size -where `x` is the width and `y` the height-, a position and a delta. Reading `x` when it holds a width is
// misleading. Split it into `Vector2<Type> { x, y }` for positions and deltas, and `Extent<Type> { width, height }` for
// sizes.
template <typename Type>
struct Rect
{
	Type x;
	Type y;
};
}  // namespace blk
