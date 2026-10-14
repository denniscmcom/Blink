// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Core.hpp"
#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Physics/Body.hpp"
#include "Engine/Physics/Collider.hpp"
#include "Engine/Scene/Directional_Light.hpp"
#include "Engine/Scene/Mesh_Ref.hpp"
#include "Engine/Scene/Point_Light.hpp"
#include "Engine/Scene/Terrain_Ref.hpp"
#include "Engine/Scene/Transform.hpp"

namespace blk
{
/// The maximum length of `Node::name` with the null-terminator.
constexpr size_t MAX_NODE_NAME_SIZE = 256;

/// The type of a `Node`.
enum class Node_Type
{
	/// Only a transform.
	SPATIAL,
	/// Populates `Node::mesh_ref`.
	MESH_REF,
	/// Populates `Node::point_light`.
	POINT_LIGHT,
	/// Populated `Node::directional_light`.
	DIRECTIONAL_LIGHT,
	/// Populates `Node::terrain_ref`.
	TERRAIN,
	/// Populates `Node::static_body`.
	STATIC_BODY,
	/// Populates `Node::dynamic_body`.
	DYNAMIC_BODY,
	/// Populates `Node::capsule_collider`.
	CAPSULE_COLLIDER,
	/// Populates `Node::heightfield_collider`.
	HEIGHTFIELD_COLLIDER,
};

enum Node_Flags : uint32_t
{
	NODE_CLEAN = 0,
	/// Recompute `Node::world_matrix`.
	NODE_TRANSFORM_DIRTY = 1 << 0,
	/// Recreate the node.
	NODE_TERRAIN_DIRTY = 1 << 1,
};

/// A node in `Scene_Graph`.
/// It does not own any heap memory.
struct Node
{
	Pool_Handle<Node> parent_handle;
	Pool_Handle<Node> first_child_handle;
	Pool_Handle<Node> last_child_handle;
	Pool_Handle<Node> prev_sibling_handle;
	Pool_Handle<Node> next_sibling_handle;

	/// Relative to `parent`.
	Transform transform;
	/// Computed by `update_node_transforms`.
	Matrix4 world_matrix;

	/// Optional node name.
	char name[MAX_NODE_NAME_SIZE];

	Node_Type type;
	uint32_t flags = NODE_TRANSFORM_DIRTY;

	// We do not use an union because we lose default initialization.

	/// Populated if `Node::type` is `Node_Type::MESH_REF`.
	Mesh_Ref mesh_ref;
	/// Populated if `Node::type` is `Node_Type::POINT_LIGHT`.
	Point_Light point_light;
	/// Populated if `Node::type` is `Node_Type::DIRECTIONAL_LIGHT`.
	Directional_Light directional_light;
	/// Populated if `Node::type` is `Node_Type::TERRAIN`.
	Terrain_Ref terrain_ref;
	/// Populated if `Node::type` is `Node_Type::STATIC_BODY`.
	Static_Body static_body;
	/// Populated if `Node::type` is `Node_Type::DYNAMIC_BODY`.
	Dynamic_Body dynamic_body;
	/// Populated if `Node::type` is `Node_Type::CAPSULE_COLLIDER`.
	Capsule_Collider capsule_collider;
	/// Populated if `Node::type` is `Node_Type::HEIGHTFIELD_COLLIDER`.
	Heightfield_Collider heightfield_collider;
};
}  // namespace blk
