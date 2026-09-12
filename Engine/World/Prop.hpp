// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

namespace blk
{
struct Node;

/// A `Prop` is an entity that interacts with other entities but it cannot be controlled by the player nor AI.
struct Prop
{
	/// A handle to its node in `Scene_Graph`.
	Pool_Handle<Node> node_handle;
};
}  // namespace blk
