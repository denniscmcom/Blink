// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Scene/Graph.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Scene/Node.hpp"

#include <stdio.h>
#include <string.h>

blk::Result
blk::create_scene_graph(Scene_Graph& scene_graph, Allocator* allocator)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	scene_graph = {};

	BLK_SUCCESS_OR_RETURN(create_pool(scene_graph.nodes, allocator, 1'024));
	BLK_SUCCESS_OR_RETURN(create_hash_map(scene_graph.hash_to_handle, allocator, 1'024, 0.75f));

	scene_graph.allocator = allocator;
	scene_graph.root = create_node(scene_graph, "Root", {});

	if (!BLK_VERIFY(scene_graph.root != POOL_HANDLE_NONE<Node>))
	{
		BLK_ERROR("Failed to create the root node\n");
		destroy_scene_graph(scene_graph);

		return Result::OUT_OF_MEMORY;
	}

	return Result::SUCCESS;
}

void
blk::destroy_scene_graph(Scene_Graph& scene_graph)
{
	// TODO (Bug): We are leaking the data owned by every node still alive. `destroy_pool` only frees the slots buffer,
	// so each node's `Mesh_Instance` is lost. `destroy_node` releases one node at a time, but `Pool` exposes no way to
	// walk its live slots and call it for each of them here.
	destroy_pool(scene_graph.nodes);
	destroy_hash_map(scene_graph.hash_to_handle);

	scene_graph = {};
}

blk::Pool_Handle<blk::Node>
blk::create_node(Scene_Graph& scene_graph, const char* name, const Pool_Handle<Node> parent)
{
	if (!BLK_VERIFY(name))
	{
		return {};
	}

	// If no `parent` is given, the node is attached to the root. When `scene_graph` has no root yet — as in
	// `create_scene_graph` — `scene_graph.root` is none too, and this node becomes the root instead.
	const Pool_Handle<Node> parent_handle = parent == POOL_HANDLE_NONE<Node> ? scene_graph.root : parent;

	// Create node structure to insert.
	Node node = {};
	node.parent_handle = parent_handle;
	node.hash = hash_fnv1a(name);

	// Copy `name` to `node.name` because `Node` owns its name.
	if (const int written = snprintf(node.name, MAX_NODE_NAME_SIZE, "%s", name);
		written < 0 || static_cast<size_t>(written) >= MAX_NODE_NAME_SIZE)
	{
		BLK_ERROR("Invalid node name\n");

		return {};
	}

	// We cannot have nodes with the same name.
	if (contains(scene_graph.hash_to_handle, node.hash))
	{
		BLK_ERROR("Cannot create node with duplicate name\n");

		return {};
	}

	const Pool_Handle<Node> node_handle = insert(scene_graph.nodes, node);

	if (!BLK_VERIFY(node_handle != POOL_HANDLE_NONE<Node>))
	{
		// Something with `scene_graph.nodes` is wrong. This should not happen unless there is a bug in its
		// implementation, or its state has been corrupted.
		BLK_ERROR("Failed to insert node\n");

		return {};
	}

	insert(scene_graph.hash_to_handle, node.hash, node_handle);

	// There is no parent to attach to, so this node is the root of `scene_graph`.
	if (parent_handle == POOL_HANDLE_NONE<Node>)
	{
		scene_graph.root = node_handle;

		return node_handle;
	}

	// We now have get the node's parent and walk its children – our future siblings – until we find an empty slot to
	// insert ourselves to.

	// `parent_handle` is a valid handle, so we get the parent node.
	Node* parent_node = get(scene_graph.nodes, parent_handle);

	if (!BLK_VERIFY(parent_node))
	{
		// This should not happen except when `scene_graph` or `scene_graph.nodes` is corrupted. We unwind both inserts
		// so the node does not stay in `scene_graph` unreachable from the tree, holding a pool slot and its name.
		BLK_ERROR("Failed to get parent node\n");

		remove(scene_graph.hash_to_handle, node.hash);
		remove(scene_graph.nodes, node_handle);

		return {};
	}

	if (Node* first_child = get(scene_graph.nodes, parent_node->first_child_handle))
	{
		// Parent node has children, so we iterate over them to find an empty slot after the last sibling.
		Node* last_sibling = first_child;

		while (Node* next_sibling = get(scene_graph.nodes, last_sibling->next_sibling_handle))
		{
			last_sibling = next_sibling;
		}

		last_sibling->next_sibling_handle = node_handle;
	}
	else
	{
		// Parent node does not have any children, so we are the first child.
		parent_node->first_child_handle = node_handle;
	}

	return node_handle;
}

void
blk::destroy_node(
	Scene_Graph& scene_graph,
	const Pool_Handle<Node> handle,
	Dyn_Array<Pool_Handle<Node>>& destroyed_handles
)
{
	Node* node = get(scene_graph.nodes, handle);

	if (!node)
	{
		// The node does not exists in `scene_graph`, so we have nothing to destroy.
		return;
	}

	// Remove the node from `scene_graph` and push its handle to `destroyed_handles`.
	remove(scene_graph.hash_to_handle, node->hash);
	push(destroyed_handles, handle);

	// Now we have to walk the node's children and siblings to remove them.

	// Remove `handle`'s children and siblings recursively. This recurses once per level of the subtree, so a scene
	// graph deep enough will overflow the stack. If that ever happens, this is why, and the fix is to walk the subtree
	// with an explicit stack instead.
	while (node->first_child_handle != POOL_HANDLE_NONE<Node>)
	{
		destroy_node(scene_graph, node->first_child_handle, destroyed_handles);
	}

	// At this point all the subtree below `handle` has been removed. So there are no orphans.

	// We have to update the handles pointing to other nodes from each node. There
	// are three possible cases:
	// 1. `handle` is the root node, so we removed the entire tree and we have to just update the `scene_graph.root`
	// handle.
	// 2. `handle` is the first child of its parent, so we point its first child to our next sibling.
	// 3. `handle` is a sibling of its parent, so we walk the siblings chain until we find ourself, then we just point
	// our previous sibling to our next sibling, essentially removing our slot from the chain.

	if (Node* parent_node = get(scene_graph.nodes, node->parent_handle); !parent_node)
	{
		// There is no parent node; we are removing the root node, so the tree is now empty.
		scene_graph.root = {};
	}
	else if (parent_node->first_child_handle == handle)
	{
		// We are the first child, so our next sibling is parent's first child.
		parent_node->first_child_handle = node->next_sibling_handle;
	}
	else
	{
		// Walk the sibling chain until we find ourself.

		Node* last_sibling_node = get(scene_graph.nodes, parent_node->first_child_handle);

		while (last_sibling_node && last_sibling_node->next_sibling_handle != handle)
		{
			last_sibling_node = get(scene_graph.nodes, last_sibling_node->next_sibling_handle);
		}

		// `last_sibling_node` is null if the chain ran out before we found ourself, which means it is broken.
		if (BLK_VERIFY(last_sibling_node))
		{
			// We are the next sibling.

			// If we have a next sibling, we point `next_sibling` of our previous sibling to our next sibling.
			// If we do not have a next sibling, `node->next_sibling` is none, so we point `next_sibling` of our
			// previous sibling to none.
			last_sibling_node->next_sibling_handle = node->next_sibling_handle;
		}
	}

	// `remove` only zeroes the slot, so we destroy the data `node` owns before removing it.
	destroy_node(*node);

	// Remove node.
	remove(scene_graph.nodes, handle);
}

blk::Result
blk::init_unique_node_name(const Scene_Graph& scene_graph, const char* base_name, char* unique_name)
{
	if (!BLK_VERIFY(base_name) || !BLK_VERIFY(unique_name))
	{
		return Result::INVALID_ARGUMENTS;
	}

	// `UINT32_MAX` is 4294967295 – 10 digits plus the null terminator.
	static constexpr size_t number_of_characters_in_uint32_max = 11;

	const size_t base_name_length = strlen(base_name);

	// `unique_name` has to fit `base_name`, the underscore, the counter and the null terminator.
	if (!BLK_VERIFY(base_name_length + 1 + number_of_characters_in_uint32_max <= MAX_NODE_NAME_SIZE))
	{
		return Result::INVALID_ARGUMENTS;
	}

	// We first copy `base_name` into `unique_name` since `base_name` does not change. If `base_name` is unique, we
	// just append the null-terminator and returns. If `base_name` it's not unique, we have to append `_` and the
	// numeric counter.
	memcpy(unique_name, base_name, base_name_length);

	if (!contains(scene_graph.hash_to_handle, hash_fnv1a(base_name)))
	{
		// `base_name` is already unique, so we append the null-terminator and return.
		unique_name[base_name_length] = '\0';

		return Result::SUCCESS;
	}

	// Now we loop incrementing a counter from 1 until we find a number that makes the node name unique. For that, we
	// have to check if the node name exists for each `base_name` plus counter combination.

	// Append an underscore before the counter.
	unique_name[base_name_length] = '_';

	bool suffix_found = false;
	uint32_t counter = 1;

	// The number of ASCII characters in `counter` written to `unique_name`.
	int counter_characters_written = 0;

	while (!suffix_found)
	{
		// Write the ASCII character of `counter` to the end of `unique_name`.
		counter_characters_written =
			snprintf(unique_name + base_name_length + 1, number_of_characters_in_uint32_max, "%u", counter);

		if (counter_characters_written < 0 ||
			static_cast<size_t>(counter_characters_written) >= number_of_characters_in_uint32_max)
		{
			return Result::INVALID_ARGUMENTS;
		}

		if (!contains(scene_graph.hash_to_handle, hash_fnv1a(unique_name)))
		{
			suffix_found = true;
		}

		counter += 1;
	}

	// `snprintf` already null-terminated `unique_name` after the counter.

	return Result::SUCCESS;
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

	const uint64_t hash = hash_fnv1a(name);

	if (contains(scene_graph.hash_to_handle, hash))
	{
		// We cannot have nodes with the same name in `scene_graph`.
		BLK_ERROR("A node with the same name already exists\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Copy the new name.
	if (const int written = snprintf(node->name, MAX_NODE_NAME_SIZE, "%s", name);
		written < 0 || static_cast<size_t>(written) >= MAX_NODE_NAME_SIZE)
	{
		BLK_ERROR("Invalid node name\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Remove the old hash – derived from the old name.
	remove(scene_graph.hash_to_handle, node->hash);

	// Update hash.
	node->hash = hash;

	// Insert the new one.
	insert(scene_graph.hash_to_handle, hash, handle);

	return Result::SUCCESS;
}

blk::Node*
blk::find_node(Scene_Graph& scene_graph, const char* name)
{
	if (!BLK_VERIFY(name))
	{
		return nullptr;
	}

	const Pool_Handle<Node>* handle = get(scene_graph.hash_to_handle, hash_fnv1a(name));

	if (!handle)
	{
		// There is no node named `name` in `scene_graph`.
		return nullptr;
	}

	return get(scene_graph.nodes, *handle);
}

blk::Node*
blk::get_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle)
{
	return get(scene_graph.nodes, handle);
}
