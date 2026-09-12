// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

namespace blk
{
struct Node;

/// An actor is an entity that can be controlled by the player or AI.
struct Actor
{
	/// A handle to its node in `Scene_Graph`.
	Pool_Handle<Node> node_handle;
	float movement_speed = 10.0f;
};
}  // namespace blk
