// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

#include <string>
#include <unordered_map>

namespace blk
{
struct Node;

struct Scene_Graph
{
	Scene_Graph();

	Pool<Node> nodes;
	Pool_Handle<Node> root;
	std::unordered_map<uint64_t, Pool_Handle<Node>> name_id_to_handle;
};

Pool_Handle<Node> create_node(Scene_Graph& scene_graph, const char* name, Pool_Handle<Node> parent);
std::string make_unique_node_name(const Scene_Graph& scene_graph, const char* base_name);
void destroy_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, std::vector<Pool_Handle<Node>>& removed_handles);
void rename_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, const char* name);
Node* get_node(const Scene_Graph& scene_graph, Pool_Handle<Node> handle);
}  // namespace blk
