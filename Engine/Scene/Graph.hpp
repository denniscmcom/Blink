// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"

namespace blk
{
enum class Result;
struct Node;
struct Allocator;
template <typename Type>
struct Dyn_Array;

/// The hierarchical representation of a `World`. It relates nodes with its parents, children and siblings.
///
/// @note This is the data structure needed by the `Renderer/`. Because of that, some data – like `Point_Light`, or
/// `Mesh_Instance` – is owned by `Node`, and other data more related to gameplay – like `Prop` – is owned by `World`.
struct Scene_Graph
{
	/// The nodes in the `World`.
	Pool<Node> nodes;
	/// The root node from which all other nodes inherits.
	Pool_Handle<Node> root;
	/// Relates the hash of each node to its pool handle. The hash is computed from the node's name.
	Hash_Map<uint64_t, Pool_Handle<Node>> hash_to_handle;
	Allocator* allocator;
};

/// Creates a default `Scene_Graph` with only a root node.
Result create_scene_graph(Scene_Graph& scene_graph, Allocator* allocator);
/// Destroys all nodes from `scene_graph`.
void destroy_scene_graph(Scene_Graph& scene_graph);

// TODO (Consistency): `create_node` and `destroy_node` are overloaded with the `Node` versions in `Node.hpp`. The ones
// here link and unlink the node from the tree, the ones there own the node's data. Overload resolution keeps them
// apart, but the shared name is confusing at the call site. Consider renaming one of the two pairs.

/// Creates a node in `scene_graph` with `name` and attaches it to `parent`. If `parent == POOL_HANDLE_NONE`, it
/// attaches it to `scene_graph.root`. If `scene_graph` has no root yet, the node becomes the root.
/// @param name A null-terminated unique string; we cannot have nodes with the same name. @see init_unique_node_name.
/// @warning It can return a `POOL_HANDLE_NONE` if it fails creating the node.
Pool_Handle<Node> create_node(Scene_Graph& scene_graph, const char* name, Pool_Handle<Node> parent);
/// Destroys a node and all its children from `scene_graph`.
/// @param destroyed_handles The node handles that have been removed.
void destroy_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, Dyn_Array<Pool_Handle<Node>>& destroyed_handles);

/// Creates a unique name based on `base_name`. It does so by appending a numeric counter to the end of `base_name`.
/// @note Nodes in `scene_graph.nodes` cannot have the same name.
/// @param base_name A null-terminated literal string. Maximum size should be `MAX_NODE_NAME_SIZE`.
/// @param unique_name A pointer to user-allocated char buffer. Maximum size should be `MAX_NODE_NAME_SIZE`. The string
/// written to it is null-terminated.
Result init_unique_node_name(const Scene_Graph& scene_graph, const char* base_name, char* unique_name);
/// Renames a node in `scene_graph`.
/// @param name A unique null-terminated literal string; we cannot have nodes with the same name. @see
/// init_unique_node_name.
Result rename_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, const char* name);

/// Returns a pointer to the `Node` named `name`, or `nullptr` if `scene_graph` has no node with that name.
Node* find_node(Scene_Graph& scene_graph, const char* name);
/// Returns a pointer to `Node` in `scene_graph.nodes` given its `handle`.
/// @note It's just a wrapper over `get(scene_graph.nodes, handle)`. You can use whatever you see fit.
Node* get_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle);
}  // namespace blk
