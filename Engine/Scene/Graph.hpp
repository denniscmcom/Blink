// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"

namespace blk
{
enum class Node_Type;
enum class Result;
struct Node;
struct Allocator;
template <typename Type>
struct Dyn_Array;

/// The hierarchical representation of a scene.
struct Scene_Graph
{
	/// The nodes in the `World`.
	Pool<Node> nodes;
	/// The root node from which all other nodes inherits.
	/// It cannot be despawned.
	Pool_Handle<Node> root;
	/// Reused traversal stack.
	Dyn_Array<Pool_Handle<Node>> stack;
	/// Scene Graph allocator.
	Allocator* allocator;
};

/// Creates a `Scene_Graph` with a single `SPATIAL` root node.
Result create_scene_graph(Scene_Graph& scene_graph, Allocator* allocator);
/// Despawns every node, frees everything they own, and destroys `scene_graph`.
void destroy_scene_graph(Scene_Graph& scene_graph);

/// Spawns a node and appends it as the last child of `parent_handle`, or of the root if `parent_handle` is none. O(1).
/// @param parent_handle If it's none, the node is parented to the root.
/// @warning Returns none on failure.
Pool_Handle<Node> spawn_node(Scene_Graph& scene_graph, Node_Type type, Pool_Handle<Node> parent_handle);

/// Despawns a single node. Its children are appended as the last children of its parent.
/// @warning The root cannot be despawned.
void despawn_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle);
/// Despawns a node and its whole subtree. Iterative, so depth is not limited by the stack.
/// @warning The root cannot be despawned.
/// @param despawned If not null, receives every removed handle so a higher layer can clean up what it attached. Notice
/// that these handles are stales because the actual node is gone.
void despawn_subtree(Scene_Graph& scene_graph, Pool_Handle<Node> handle, Dyn_Array<Pool_Handle<Node>>* despawned);

/// Unlinks `handle` from its parent.
/// Its own subtree stays attached to it.
/// @warning The root cannot be unlinked.
void unlink_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle);
/// Appends `handle` as the last child of `parent_handle`. O(depth of `parent_handle`).
/// Its own subtree stays attached to it.
/// @param parent_handle If it's none, the node is linked to the root.
/// @warning `handle` must have no parent, so call `unlink_node` first. It cannot be linked under its own subtree.
void link_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, Pool_Handle<Node> parent_handle);

/// Renames a node.
/// @param name A unique null-terminated literal string.
Result rename_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, const char* name);

/// Returns the node, or `nullptr` if `handle` is none or stale.
Node* get_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle);

/// Updates `Node::world_matrix` for every node, root-down.
/// @note Call once per frame.
void update_node_transforms(Scene_Graph& scene_graph);
}  // namespace blk
