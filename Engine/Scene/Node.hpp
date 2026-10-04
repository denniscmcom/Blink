// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Pool.hpp"
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

	// We do not use an union because we lose default initialization.

	/// Populated if `Node::type` is `Node_Type::MESH_REF`.
	Mesh_Ref mesh_ref;
	/// Populated if `Node::type` is `Node_Type::POINT_LIGHT`.
	Point_Light point_light;
	/// Populated if `Node::type` is `Node_Type::DIRECTIONAL_LIGHT`.
	Directional_Light directional_light;
	/// Populated if `Node::type` is `Node_Type::TERRAIN`.
	Terrain_Ref terrain_ref;
};
}  // namespace blk
