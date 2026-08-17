// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Scene/Scene_Graph.hpp"

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Scene/Node.hpp"

blk::Scene_Graph::Scene_Graph()
{
	root = create_node(*this, "Root", {});
}

blk::Pool_Handle<blk::Node>
blk::create_node(Scene_Graph& scene_graph, const char* name, const Pool_Handle<Node> parent)
{
	Node node = {};
	node.parent_handle = parent;
	node.name = name;
	node.name_id = hash_fnv1a(name);

	if (auto search = scene_graph.name_id_to_handle.find(node.name_id); search != scene_graph.name_id_to_handle.end())
	{
		BLK_ERROR("Cannot create node with duplicate name\n");

		return {};
	}

	const Pool_Handle<Node> node_handle = scene_graph.nodes.insert(node);
	scene_graph.name_id_to_handle.insert({node.name_id, node_handle});

	if (parent == POOL_HANDLE_NONE<Node>)
	{
		scene_graph.root = node_handle;

		return node_handle;
	}

	Node* parent_node = scene_graph.nodes.get(parent);

	if (!parent_node)
	{
		BLK_ERROR("Failed to find parent node\n");

		return {};
	}

	if (Node* first_child = scene_graph.nodes.get(parent_node->first_child_handle))
	{
		// Parent node has children, so we iterate over them to find an empty slot after the last sibling.
		Node* last_sibling = first_child;

		while (Node* next_sibling = scene_graph.nodes.get(last_sibling->next_sibling_handle))
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
	std::vector<Pool_Handle<Node>>& removed_handles
)
{
	const Node* node = scene_graph.nodes.get(handle);

	if (!node)
	{
		return;
	}

	removed_handles.push_back(handle);
	scene_graph.name_id_to_handle.erase(node->name_id);

	// Remove node children first.
	while (node->first_child_handle != POOL_HANDLE_NONE<Node>)
	{
		destroy_node(scene_graph, node->first_child_handle, removed_handles);
	}

	// Remove link to parent node.
	if (Node* parent_node = scene_graph.nodes.get(node->parent_handle); !parent_node)
	{
		// We are removing the root node, so the tree is now empty.
		scene_graph.root = {};
	}
	else if (parent_node->first_child_handle == handle)
	{
		// We are the first child, so our next sibling is parent's first child.
		parent_node->first_child_handle = node->next_sibling_handle;
	}
	else
	{
		Node* last_sibling_node = scene_graph.nodes.get(parent_node->first_child_handle);
		BLK_CHECK(last_sibling_node);

		while (last_sibling_node->next_sibling_handle != handle)
		{
			last_sibling_node = scene_graph.nodes.get(last_sibling_node->next_sibling_handle);
			BLK_CHECK(last_sibling_node);
		}

		// We are the next sibling.

		// If we have a next sibling, we point `next_sibling` of our previous sibling to our next sibling.
		// If we do not have a next sibling, `node->next_sibling` is none, so we point `next_sibling` of our previous
		// sibling to none.
		last_sibling_node->next_sibling_handle = node->next_sibling_handle;
	}

	// Remove node.
	scene_graph.nodes.remove(handle);
}

std::string
blk::make_unique_node_name(const Scene_Graph& scene_graph, const char* base_name)
{
	if (!scene_graph.name_id_to_handle.contains(hash_fnv1a(base_name)))
	{
		return base_name;
	}

	std::string name = base_name;
	bool suffix_found = false;
	uint32_t suffix = 1;

	while (!suffix_found)
	{
		name = base_name;
		name += '_';
		name += std::to_string(suffix);

		if (!scene_graph.name_id_to_handle.contains(hash_fnv1a(name.c_str())))
		{
			suffix_found = true;
		}

		suffix += 1;
	}

	return name;
}

void
blk::rename_node(Scene_Graph& scene_graph, Pool_Handle<Node> handle, const char* name)
{
	Node* node = scene_graph.nodes.get(handle);

	if (!node)
	{
		return;
	}

	const uint64_t name_id = hash_fnv1a(name);

	if (scene_graph.name_id_to_handle.contains(name_id))
	{
		BLK_ERROR("Name already exists\n");

		return;
	}

	scene_graph.name_id_to_handle.erase(node->name_id);

	node->name = name;
	node->name_id = name_id;

	scene_graph.name_id_to_handle.insert({node->name_id, handle});
}

blk::Node*
blk::get_node(const Scene_Graph& scene_graph, Pool_Handle<Node> handle)
{
	return scene_graph.nodes.get(handle);
}
