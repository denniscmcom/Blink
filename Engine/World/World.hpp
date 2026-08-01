// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/World/Actor.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/Prop.hpp"

namespace blk
{
struct Node;

enum class Entity_Type : uint8_t
{
	Actor,
	Camera,
	Prop,
};

struct Entity_Instance
{
	Entity_Type type;
	uint32_t id = UINT32_MAX;
	uint32_t version = UINT32_MAX;
};

struct World
{
	Pool<Actor> actors;
	Pool<Camera> cameras;
	Pool<Prop> props;
	Pool_Handle<Camera> active_camera_handle;
	Scene_Graph scene_graph;

	std::unordered_map<Pool_Handle<Node>, Entity_Instance, Pool_Handle_Hash<Node>> node_handle_to_entity_instance;
};

Pool_Handle<Actor> spawn_actor(World& world, const char* name, Pool_Handle<Node> parent);
Pool_Handle<Camera> spawn_camera(World& world, const char* name, Pool_Handle<Node> parent);
Pool_Handle<Prop> spawn_prop(World& world, const char* name, Pool_Handle<Node> parent);

void despawn_actor(World& world, Pool_Handle<Actor> handle);
void despawn_camera(World& world, Pool_Handle<Camera> handle);
void despawn_prop(World& world, Pool_Handle<Prop> handle);
void despawn_node(World& world, Pool_Handle<Node> handle);

Pool_Handle<Actor> find_actor(const World& world, const char* name);
Pool_Handle<Camera> find_camera(const World& world, const char* name);
Pool_Handle<Prop> find_prop(const World& world, const char* name);
}  // namespace blk
