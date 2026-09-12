// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Scene/Light.hpp"
#include "Engine/Scene/Mesh_Instance.hpp"
#include "Engine/Scene/Transform.hpp"

namespace blk
{
/// The maximum length of `Node::name` with the null-terminator.
constexpr size_t MAX_NODE_NAME_SIZE = 256;

// TODO (Consistency): `Node_Type` has no sentinel for a node that is neither, so `Node node = {}` zero-initializes
// `type` to `MESH_INSTANCE`. Every node spawned for a `Camera` or an `Actor` therefore claims to be a mesh instance
// with an empty `Mesh_Instance`. It is harmless while callers check `mesh_handles.count` first, but the field is
// lying. Add a `NONE` value at the beginning, as `Key` and `Event_Type` already do.

/// The type of a `Node` in `World`.
/// @see `Node`.
enum class Node_Type
{
	/// @see `Mesh_Instance`.
	MESH_INSTANCE,
	/// @see `Point_Light`.
	POINT_LIGHT,
};

/// The low-level representation of an entity in a `World`. A `Node` can have additional data specific for a type of
/// entity. For example, a `Point_Light` in a `World` is a `Node` with its field `Node::point_light` populated.
struct Node
{
	/// The handle of its parent node.
	///
	/// If `Node` is root, then `parent_handle` is `POOL_HANDLE_NONE<Node>`.
	Pool_Handle<Node> parent_handle;
	/// The handle of its first child.
	///
	/// A `Node` can have many children, we represent that by storing a handle to the next sibling in each `Node`. For
	/// example, to walk the children of this `Node`, we get the first child node and then iterate over the next sibling
	/// until `next_sibling_handle` is `POOL_HANDLE_NONE<Node>`.
	Pool_Handle<Node> first_child_handle;
	/// The handle of its next sibling. If it does not exist, `next_sibling_handle` is `POOL_HANDLE_NONE<Node>`.
	Pool_Handle<Node> next_sibling_handle;

	/// Spatial transform.
	Transform transform;
	/// The name of the node.
	///
	/// @warning It should be unique in `Scene_Graph`.
	/// @note This name is used to compute `Node::hash`.
	char name[MAX_NODE_NAME_SIZE];
	/// The hash of the node. Used to get nodes from the `Scene_Graph` efficiently. We compare `uint64_t` instead of
	/// `char[]`.
	uint64_t hash;
	/// The type of `Node`.
	Node_Type type;

	// TODO (Performance): Every `Node` carries both fields even though `Node_Type` says only one of them is populated.
	// A point light pays for two `Dyn_Array` that `create_node` allocates and never fills, and a mesh instance pays for
	// an unused color. A union would remove both, at the cost of having to read `type` before touching either field.

	/// If `Node::type` is `Node_Type::MESH_INSTANCE`, this field is populated.
	Mesh_Instance mesh_instance;
	/// If `Node::type` is `Node_Type::POINT_LIGHT`, this field is populated.
	Point_Light point_light;
};

// TODO (Consistency): `create_node` and `destroy_node` are overloaded with the `Scene_Graph` versions in `Graph.hpp`.
// The ones here own the node's data, the ones there link and unlink the node from the tree. Overload resolution keeps
// them apart, but the shared name is confusing at the call site. Consider renaming one of the two pairs.

/// Creates a `Node` and the data it owns.
/// @param mesh_capacity The number of meshes its `Mesh_Instance` can hold.
Result create_node(Node& node, Allocator* allocator, size_t mesh_capacity);
/// Destroys the data owned by `node`. It does not unlink `node` from its `Scene_Graph`.
void destroy_node(Node& node);
}  // namespace blk
