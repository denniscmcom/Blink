#pragma once

#include "Core/Pool.hpp"

namespace blk
{
struct Actor
{
	Pool_Handle<Node> node_handle;
	float movement_speed = 10.0f;
};
}  // namespace blk
