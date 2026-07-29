#pragma once

#include "Core/Pool.hpp"

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
void destroy_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, std::vector<Pool_Handle<Node>>& removed_handles);
std::string make_unique_node_name(const Scene_Graph& scene_graph, const char* base_name);
void rename_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, const char* name);
}  // namespace blk
