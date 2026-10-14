// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Scene/Graph.hpp"

#include "Engine/Core/Core.hpp"
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Scene/Node.hpp"

#include <tracy/Tracy.hpp>

#include <stdio.h>

blk::Result
blk::create_scene_graph(Scene_Graph& scene_graph, Allocator* allocator)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	scene_graph = {};

	if (const Result result = create_pool(scene_graph.nodes, allocator, 1'024 * 4); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create node pool\n");

		return result;
	}

	if (const Result result = create_dyn_array(scene_graph.stack, allocator, 256); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create scene graph stack array\n");
		destroy_scene_graph(scene_graph);

		return result;
	}

	scene_graph.allocator = allocator;

	// The root is inserted directly because `spawn_node` requires a parent. It lives until `destroy_scene_graph`.

	Node root = {};
	root.type = Node_Type::SPATIAL;
	root.world_matrix = init_identity_matrix();

	scene_graph.root = insert(scene_graph.nodes, root);

	BLK_IF_NOT_SUCCESS(rename_node(scene_graph, scene_graph.root, "Root"))
	{
		BLK_ERROR("Failed to rename root node\n");
	}

	if (!BLK_VERIFY(scene_graph.root != POOL_HANDLE_NONE<Node>))
	{
		destroy_scene_graph(scene_graph);

		return Result::OUT_OF_MEMORY;
	}

	return Result::SUCCESS;
}

void
blk::destroy_scene_graph(Scene_Graph& scene_graph)
{
	destroy_pool(scene_graph.nodes);
	destroy_dyn_array(scene_graph.stack);

	scene_graph = {};
}

blk::Pool_Handle<blk::Node>
blk::spawn_node(Scene_Graph& scene_graph, Physics* physics, Node_Type type, const Pool_Handle<Node> parent_handle)
{
	// Initialize node structure to insert.

	Node node = {};
	node.type = type;
	node.parent_handle = parent_handle == POOL_HANDLE_NONE<Node> ? scene_graph.root : parent_handle;
	node.world_matrix = init_identity_matrix();

	// The root always exists, but an explicit `parent_handle` may be stale. We verify it before inserting, so a failure
	// does not leave an unlinked node in the pool.

	if (!BLK_VERIFY(get(scene_graph.nodes, node.parent_handle)))
	{
		return {};
	}

	// Initialize physics data is needed.

	const bool is_physics_node =
		type == Node_Type::STATIC_BODY || type == Node_Type::DYNAMIC_BODY || type == Node_Type::CAPSULE_COLLIDER;

	if (is_physics_node && !BLK_VERIFY(physics))
	{
		return {};
	}

	// Create static body.

	if (type == Node_Type::STATIC_BODY)
	{
		node.static_body = create_static_body(*physics, {});
	}

	// Create dynamic body.

	if (type == Node_Type::DYNAMIC_BODY)
	{
		node.dynamic_body = create_dynamic_body(*physics, {});
	}

	// Create capsule collider.

	if (type == Node_Type::CAPSULE_COLLIDER)
	{
		const Node* parent = get_node(scene_graph, parent_handle);
		BLK_CHECK(parent);

		if (parent->type == Node_Type::STATIC_BODY)
		{
			node.capsule_collider = create_capsule_collider(parent->static_body);
		}
		else if (parent->type == Node_Type::DYNAMIC_BODY)
		{
			node.capsule_collider = create_capsule_collider(parent->dynamic_body);
		}
		else
		{
			BLK_ERROR("A capsule collider can only be parented to a static or dynamic physics body\n");

			return {};
		}
	}

	const Pool_Handle<Node> node_handle = insert(scene_graph.nodes, node);

	if (!BLK_VERIFY(node_handle != POOL_HANDLE_NONE<Node>))
	{
		// Something with `scene_graph.nodes` is wrong.
		// This should not happen unless there is a bug in its implementation, or its state has been corrupted.

		return {};
	}

#ifdef BLK_EDITOR
	// Add default editor node name.
	switch (node.type)
	{
	case Node_Type::SPATIAL:
		rename_node(scene_graph, node_handle, "Spatial");
		break;
	case Node_Type::MESH_REF:
		rename_node(scene_graph, node_handle, "Mesh_Ref");
		break;
	case Node_Type::POINT_LIGHT:
		rename_node(scene_graph, node_handle, "Point_Light");
		break;
	case Node_Type::DIRECTIONAL_LIGHT:
		rename_node(scene_graph, node_handle, "Directional_Light");
		break;
	case Node_Type::TERRAIN:
		rename_node(scene_graph, node_handle, "Terrain");
		break;
	case Node_Type::STATIC_BODY:
		rename_node(scene_graph, node_handle, "Static_Body");
		break;
	case Node_Type::DYNAMIC_BODY:
		rename_node(scene_graph, node_handle, "Dynamic_Body");
		break;
	case Node_Type::CAPSULE_COLLIDER:
		rename_node(scene_graph, node_handle, "Capsule_Collider");
		break;
	case Node_Type::HEIGHTFIELD_COLLIDER:
		rename_node(scene_graph, node_handle, "Heightfield_Collider");
		break;
	}
#endif

	// After inserting the node, we fetch the pointer to itself and its parent.

	Node* new_node = get(scene_graph.nodes, node_handle);
	Node* parent = get(scene_graph.nodes, node.parent_handle);

	BLK_CHECK(new_node);
	BLK_CHECK(parent);

	// We are inserting ourself as the last sibling, so our previous sibling is the last child of our parent.
	new_node->prev_sibling_handle = parent->last_child_handle;

	if (Node* last_child = get(scene_graph.nodes, parent->last_child_handle))
	{
		// Now, we are the next sibling of our previous sibling.
		last_child->next_sibling_handle = node_handle;
	}
	else
	{
		// We are the first child of our parent.
		parent->first_child_handle = node_handle;
	}

	// Finally, we are the last child of our parent.
	parent->last_child_handle = node_handle;

	return node_handle;
}

void
blk::despawn_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle)
{
	const Node* node = get(scene_graph.nodes, handle);

	if (!node)
	{
		// We have nothing to despawn.
		return;
	}

	if (!BLK_VERIFY(handle != scene_graph.root))
	{
		return;
	}

	// Move our children up to our parent.

	Pool_Handle<Node> child_handle = node->first_child_handle;

	while (const Node* child = get(scene_graph.nodes, child_handle))
	{
		// `unlink_node` clears the sibling links, so we read the next child first.
		const Pool_Handle<Node> next_child_handle = child->next_sibling_handle;

		unlink_node(scene_graph, child_handle);
		link_node(scene_graph, child_handle, node->parent_handle);

		child_handle = next_child_handle;
	}

	// Despawn the node, which has no children now.

	unlink_node(scene_graph, handle);
	remove(scene_graph.nodes, handle);
}

void
blk::despawn_subtree(Scene_Graph& scene_graph, Pool_Handle<Node> handle, Dyn_Array<Pool_Handle<Node>>* despawned)
{
	const Node* node = get(scene_graph.nodes, handle);

	if (!node)
	{
		// We have nothing to despawn.
		return;
	}

	if (!BLK_VERIFY(handle != scene_graph.root))
	{
		return;
	}

	// Detach the subtree first.
	unlink_node(scene_graph, handle);

	// Iterate over the detached subtree and despawn every node.
	// We use a pre-allocated stack to despawn each node in the subtree without recursion.

	Dyn_Array<Pool_Handle<Node>>& stack = scene_graph.stack;

	empty(stack);
	push(stack, handle);

	while (stack.count > 0)
	{
		stack.count -= 1;

		const Pool_Handle<Node> current_node_handle = stack.buffer[stack.count];
		Node* current_node = get(scene_graph.nodes, current_node_handle);

		if (!BLK_VERIFY(current_node))
		{
			// This should not happen.
			continue;
		}

		// Push node children onto the stack to despawn them later.
		push_children(scene_graph, *current_node);

		// Destroy physics data.
		// A body is despawned before its colliders, and destroying it destroys their shapes too.

		switch (current_node->type)
		{
		case Node_Type::STATIC_BODY:
			destroy_static_body(current_node->static_body);
			break;
		case Node_Type::DYNAMIC_BODY:
			destroy_dynamic_body(current_node->dynamic_body);
			break;
		case Node_Type::HEIGHTFIELD_COLLIDER:
			destroy_heightfield_collider(current_node->heightfield_collider);
			break;
		default:
			break;
		}

		// Despawn current node.

		if (despawned)
		{
			push(*despawned, current_node_handle);
		}

		remove(scene_graph.nodes, current_node_handle);
	}
}

void
blk::unlink_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle)
{
	if (!BLK_VERIFY(handle != scene_graph.root))
	{
		return;
	}

	// Get node to unlink.

	Node* node = get(scene_graph.nodes, handle);

	if (!node)
	{
		// Nothing to unlink.
		return;
	}

	// Get its parent. Every node except the root has one.

	Node* parent = get(scene_graph.nodes, node->parent_handle);
	BLK_CHECK(parent);

	// Check if we have previous sibling.

	if (Node* prev_sibling = get(scene_graph.nodes, node->prev_sibling_handle))
	{
		// We have a previous sibling, so its next sibling is our next sibling.
		prev_sibling->next_sibling_handle = node->next_sibling_handle;
	}
	else
	{
		// We are our parent's first child, so our next sibling is now the first child.
		parent->first_child_handle = node->next_sibling_handle;
	}

	// Check if we have next sublings.

	if (Node* next_sibling = get(scene_graph.nodes, node->next_sibling_handle))
	{
		// We have a next sibling, so its previous sibling is our previous sibling.
		next_sibling->prev_sibling_handle = node->prev_sibling_handle;
	}
	else
	{
		// We are our parent's last child, so our previous sibling is now the last child.
		parent->last_child_handle = node->prev_sibling_handle;
	}

	// Orphan the node.

	node->parent_handle = {};
	node->prev_sibling_handle = {};
	node->next_sibling_handle = {};
}

void
blk::link_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, Pool_Handle<Node> parent_handle)
{
	if (!BLK_VERIFY(handle != scene_graph.root))
	{
		return;
	}

	// Get node to link.

	Node* node = get(scene_graph.nodes, handle);

	if (!node)
	{
		// Nothing to link.
		return;
	}

	if (!BLK_VERIFY(node->parent_handle == POOL_HANDLE_NONE<Node>))
	{
		// Cannot link a node that has a parent.

		return;
	}

	// Get its new parent.

	const Pool_Handle<Node> new_parent_handle =
		parent_handle == POOL_HANDLE_NONE<Node> ? scene_graph.root : parent_handle;
	Node* parent = get(scene_graph.nodes, new_parent_handle);

	if (!BLK_VERIFY(parent))
	{
		return;
	}

	// Linking under our own subtree would make a cycle detached from the root. If the new parent is in our subtree,
	// walking up from it reaches us.

	Pool_Handle<Node> ancestor_handle = new_parent_handle;

	while (const Node* ancestor = get(scene_graph.nodes, ancestor_handle))
	{
		if (!BLK_VERIFY(ancestor_handle != handle))
		{
			// Cannot link a node under its own subtree.

			return;
		}

		ancestor_handle = ancestor->parent_handle;
	}

	// We are inserting ourself as the last sibling, so our previous sibling is the last child of our parent.

	node->parent_handle = new_parent_handle;
	node->prev_sibling_handle = parent->last_child_handle;

	if (Node* last_child = get(scene_graph.nodes, parent->last_child_handle))
	{
		// Now, we are the next sibling of our previous sibling.
		last_child->next_sibling_handle = handle;
	}
	else
	{
		// We are the first child of our parent.
		parent->first_child_handle = handle;
	}

	// Finally, we are the last child of our parent.
	parent->last_child_handle = handle;

	// Our transform is now relative to a different parent.
	node->flags |= NODE_TRANSFORM_DIRTY;
}

blk::Result
blk::rename_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, const char* name)
{
	if (!BLK_VERIFY(name))
	{
		return Result::INVALID_ARGUMENTS;
	}

	Node* node = get(scene_graph.nodes, handle);

	if (!node)
	{
		// Node does not exists in `scene_graph`, so we have nothing to rename.
		return Result::SUCCESS;
	}

	// Copy the new name.
	BLK_IF_NOT_SNPRINTF(node->name, MAX_NODE_NAME_SIZE, "%s", name)
	{
		BLK_ERROR("Invalid node name\n");

		return Result::INVALID_ARGUMENTS;
	}

	return Result::SUCCESS;
}

blk::Node*
blk::get_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle)
{
	return get(scene_graph.nodes, handle);
}

void
blk::push_children(Scene_Graph& scene_graph, const Node& node)
{
	// Set child handle as the first child to start iterating over the list.
	Pool_Handle<Node> child_handle = node.first_child_handle;

	// Iterate over the sibling list pushing each sibling to the stack.
	while (const Node* child = get(scene_graph.nodes, child_handle))
	{
		push(scene_graph.stack, child_handle);
		child_handle = child->next_sibling_handle;
	}
}

void
blk::move_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, Vector3 delta)
{
	Node* node = get_node(scene_graph, handle);

	if (!node)
	{
		return;
	}

	node->transform.position += delta;
	node->flags |= NODE_TRANSFORM_DIRTY;
}

void
blk::rotate_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, Vector3 delta)
{
	Node* node = get_node(scene_graph, handle);

	if (!node)
	{
		return;
	}

	node->transform.rotation += delta;
	node->flags |= NODE_TRANSFORM_DIRTY;
}
