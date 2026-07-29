#pragma once

#include "Core/Pool.hpp"
#include "Scene/Light.hpp"
#include "Scene/Mesh_Instance.hpp"
#include "Scene/Transform.hpp"

#include <optional>
#include <string>

namespace blk
{
struct Node
{
	Pool_Handle<Node> parent_handle;
	Pool_Handle<Node> first_child_handle;
	Pool_Handle<Node> next_sibling_handle;

	Transform transform;
	std::string name;
	uint64_t name_id;

	std::optional<Mesh_Instance> mesh_instance;
	std::optional<Point_Light> point_light;
};
}  // namespace blk
