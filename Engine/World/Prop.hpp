#pragma once

#include "Core/Pool.hpp"
#include "Scene/Node.hpp"

namespace blk
{
struct Prop
{
	Pool_Handle<Node> node_handle;
};
}  // namespace blk
