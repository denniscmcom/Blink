// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/World/Actor.hpp"
#include "Engine/World/Atmosphere.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/Prop.hpp"
#include "Engine/World/Stream.hpp"
#include "Engine/World/Terrain.hpp"

namespace blk
{
struct Serial;
struct Mesh;
struct Node;
struct Allocator;
enum class Result;

/// The type of gameplay data that can be attached to a `Node`.
/// Add one only when a `Node` needs data or behaviour.
enum class Entity_Type : uint8_t
{
	NONE,
	ACTOR,
	CAMERA,
	PROP,
};

/// A reference used to associate any entity with its `Node`.
struct Entity_Ref
{
	Entity_Type type;
	/// The id of the entity in the pool to compose `Pool_Handle`.
	/// `SIZE_MAX` is the sentinel value that represents an invalid id.
	/// @warning `SIZE_MAX` should match `Pool_Handle::id`.
	size_t id = SIZE_MAX;
	/// The version of the entity slot in the pool to compose `Pool_Handle`.
	/// `SIZE_MAX` is the sentinel value that represents an invalid version.
	/// @warning `SIZE_MAX` should match `Pool_Handle::version`.
	size_t version = SIZE_MAX;
};

/// Global world settings.
struct World_Settings
{
	Atmosphere_Settings atmosphere;
	Terrain_Settings terrain;
	Stream_Settings stream;
};

/// Bits for `World::flags`.
enum World_Flag_Bits : uint32_t
{
	/// A value that the terrain was built from has changed.
	WORLD_TERRAIN_DIRTY_BIT = 1 << 0,
};

/// A `Scene_Graph` plus the gameplay data attached to some of its nodes.
///
/// @warning Despawning nodes with `despawn_node(world.scene_graph, ...)` or `despawn_subtree(world.scene_graph, ...)`
/// does not despawn their attached entities. Use the `World` overloads instead.
struct World
{
	Pool<Actor> actors;
	Pool<Camera> cameras;
	Pool<Prop> props;

	/// The handle of the active camera.
	/// @warning The camera should exist in `cameras`.
	Pool_Handle<Camera> active_camera_handle;

	/// Association between a node handle and its entity.
	Hash_Map<Pool_Handle<Node>, Entity_Ref> node_handle_to_entity;
	/// Stale handles of despawned nodes whose entities are still pending clean up.
	Dyn_Array<Pool_Handle<Node>> despawned_nodes;

	/// The focus chunk at local position (0, 0, 0).
	/// Every position in the world is relative to it.
	Chunk_Coord origin;
	/// Loaded chunks.
	Dyn_Array<Chunk> chunks;

	Scene_Graph scene_graph;
	World_Settings settings;
	/// Combination of `World_Flag_Bits`. `update_world` handles and clears them.
	uint32_t flags;

	Allocator* allocator;
};

// TODO (Consistency): The output comes last here, while `create_scene_graph`, `create_pool` and the `Result.hpp` example
// put it first, followed by the allocator. Decide on one order.

/// Creates an empty `World`.
Result create_world(Allocator* allocator, const World_Settings& settings, World& world);
/// Destroys `world`.
void destroy_world(World& world);
/// Rebase the world origin based on the active camera position, and loads/unloads chunks.
/// @param node_offset The transform offset applied to root nodes if a rebase happened.
void update_world(World& world, Vector3& node_offset);

/// Spawns an actor into `world` and returns its handle.
/// @param parent_handle If it's none, the actor is parented to the root.
/// @warning Return none on failure.
Pool_Handle<Actor> spawn_actor(World& world, Pool_Handle<Node> parent_handle);
/// Spawns a camera into `world` and returns its handle.
/// @param parent_handle If it's none, the camera is parented to the root.
/// @warning Return none on failure.
Pool_Handle<Camera> spawn_camera(World& world, Pool_Handle<Node> parent_handle);
/// Spawns a prop into `world` and returns its handle.
/// @param parent_handle If it's none, the prop is parented to the root.
/// @warning Return none on failure.
Pool_Handle<Prop> spawn_prop(World& world, Pool_Handle<Node> parent_handle);

/// Despawns an actor from `world`.
void despawn_actor(World& world, Pool_Handle<Actor> handle);
/// Despawns a camera from `world`.
void despawn_camera(World& world, Pool_Handle<Camera> handle);
/// Despawns a prop from `world`.
void despawn_prop(World& world, Pool_Handle<Prop> handle);

/// Despawns a single node and its attached entity, and attaches its children to its parent.
/// @warning The root cannot be despawned.
void despawn_node(World& world, Pool_Handle<Node> handle);
/// Despawns a node, its whole subtree, and every entity attached to them.
/// @warning The root cannot be despawned.
void despawn_subtree(World& world, Pool_Handle<Node> handle);

/// Despawns every loaded chunk from `world`.
void despawn_chunks(World& world);

/// Returns the actor, or `nullptr` if `handle` is none or stale.
Actor* get_actor(World& world, Pool_Handle<Actor> handle);
/// Returns the camera, or `nullptr` if `handle` is none or stale.
Camera* get_camera(World& world, Pool_Handle<Camera> handle);
/// Returns the prop, or `nullptr` if `handle` is none or stale.
Prop* get_prop(World& world, Pool_Handle<Prop> handle);
}  // namespace blk
