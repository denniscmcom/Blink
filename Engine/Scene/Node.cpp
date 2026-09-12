// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Scene/Node.hpp"

#include "Engine/Platform/Result.hpp"
#include "Engine/Scene/Mesh_Instance.hpp"

blk::Result
blk::create_node(Node& node, Allocator* allocator, const size_t mesh_capacity)
{
	node = {};

	return create_mesh_instance(node.mesh_instance, allocator, mesh_capacity);
}

void
blk::destroy_node(Node& node)
{
	destroy_mesh_instance(node.mesh_instance);

	node = {};
}
