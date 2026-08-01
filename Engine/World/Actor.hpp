// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

namespace blk
{
struct Node;

struct Actor
{
	Pool_Handle<Node> node_handle;
	float movement_speed = 10.0f;
};
}  // namespace blk
