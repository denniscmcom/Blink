// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/World/Actor.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/Prop.hpp"

namespace blk
{
struct Mesh;
struct Node;
struct Allocator;
enum class Result;

/// The type of a `Entity` in `World`.
enum class Entity_Type : uint8_t
{
	/// @see `Actor`.
	ACTOR,
	/// @see `Camera`.
	CAMERA,
	/// @see `Prop`.
	PROP,
};

/// A generic entity used to associate any `Entity` with its `Node`.
struct Entity
{
	Entity_Type type = Entity_Type::ACTOR;
	/// The id of the entity in the pool to compose `Pool_Handle`.
	/// `SIZE_MAX` is the sentinel value that represents an invalid id.
	/// @warning `SIZE_MAX` should match `Pool_Handle::id`.
	size_t id = SIZE_MAX;
	/// The version of the entity slot in the pool to compose `Pool_Handle`.
	/// `SIZE_MAX` is the sentinel value that represents an invalid version.
	/// @warning `SIZE_MAX` should match `Pool_Handle::version`.
	size_t version = SIZE_MAX;
};

/// A `World` – commonly named level or scene in other engines.
/// @note Only one `World` could be loaded at a time.
struct World
{
	/// Pool of `Actor` entities.
	Pool<Actor> actors;
	/// Pool of `Camera` entities.
	Pool<Camera> cameras;
	/// Pool of `Prop` entities.
	Pool<Prop> props;
	/// The handle of the active camera.
	/// @warning The camera should exist in `cameras` and only one camera can be activated at a time.
	Pool_Handle<Camera> active_camera_handle;
	/// The hierarchical representation of `World`.
	Scene_Graph scene_graph;
	/// Associates a node handle of each `Node` in `scene_graph` with its `Entity` in one of the pool of entities
	/// (`actors`, `cameras`).
	Hash_Map<Pool_Handle<Node>, Entity> node_handle_to_entity;
	/// Pointer to allocator.
	Allocator* allocator;
};

/// Creates an empty `World`.
Result create_world(Allocator* allocator, World& world);
/// Destroys `world`.
void destroy_world(World& world);

/// Spawns an actor in `world` and returns its handle.
/// @param name A null-terminated string. It is copied, and it does not have to be unique — this function makes it so.
/// @param parent A valid handle to its parent node.
/// @warning It can return a `POOL_HANDLE_NONE` if it fails creating the `Actor`.
Pool_Handle<Actor> spawn_actor(World& world, const char* name, Pool_Handle<Node> parent);
/// Spawns a camera in `World` and returns its handle.
/// @param name A null-terminated string. It is copied, and it does not have to be unique — this function makes it so.
/// @param parent A valid handle to its parent node.
/// @warning It can return a `POOL_HANDLE_NONE` if it fails creating the `Camera`.
Pool_Handle<Camera> spawn_camera(World& world, const char* name, Pool_Handle<Node> parent);
/// Spawns a prop in `world` and returns its handle.
/// @param name A null-terminated string. It is copied, and it does not have to be unique — this function makes it so.
/// @param parent A valid handle to its parent node.
/// @warning It can return a `POOL_HANDLE_NONE` if it fails creating the `Prop`.
Pool_Handle<Prop> spawn_prop(World& world, const char* name, Pool_Handle<Node> parent);

/// Despawns an `Actor` from `world`.
void despawn_actor(World& world, Pool_Handle<Actor> handle);
/// Despawns a `Camera` from `world`.
void despawn_camera(World& world, Pool_Handle<Camera> handle);
/// Despawns a `Prop` from `world`.
void despawn_prop(World& world, Pool_Handle<Prop> handle);
/// Despawns an entity by its `Node` from `world`.
void despawn_entity_by_node(World& world, Pool_Handle<Node> handle);

/// Finds an `Actor` in `world` by its `name`.
/// It returns `POOL_HANDLE_NONE` if `Actor` does not exist.
/// @param name A null-terminated string.
Pool_Handle<Actor> find_actor(World& world, const char* name);
/// Finds a `Camera` in `world` by its `name`.
/// It returns `POOL_HANDLE_NONE` if `Camera` does not exist.
/// @param name A null-terminated string.
Pool_Handle<Camera> find_camera(World& world, const char* name);
/// Finds an `Prop` in `world` by its `name`.
/// It returns `POOL_HANDLE_NONE` if `Prop` does not exist.
/// @param name A null-terminated string.
Pool_Handle<Prop> find_prop(World& world, const char* name);

/// Returns a pointer to the `Actor` associated with `handle`.
/// It returns `nullptr` if `Actor` does not exist in `world.actors`.
Actor* get_actor(World& world, Pool_Handle<Actor> handle);
/// Returns a pointer to the `Camera` associated with `handle`.
/// It returns `nullptr` if `Camera` does not exist in `world.cameras`.
Camera* get_camera(World& world, Pool_Handle<Camera> handle);
/// Returns a pointer to the `Prop` associated with `handle`.
/// It returns `nullptr` if `Prop` does not exist in `world.props`.
Prop* get_prop(World& world, Pool_Handle<Prop> handle);

/// Returns a pointer to the `Actor`'s `Node`.
/// It returns `nullptr` if `Actor` does not exist in `world.actors`.
Node* get_node(World& world, Pool_Handle<Actor> handle);
/// Returns a pointer to the `Camera`'s `Node`.
/// It returns `nullptr` if `Camera` does not exist in `world.cameras`.
Node* get_node(World& world, Pool_Handle<Camera> handle);
/// Returns a pointer to the `Prop`'s `Node`.
/// It returns `nullptr` if `Prop` does not exist in `world.props`.
Node* get_node(World& world, Pool_Handle<Prop> handle);

/// Attaches a `mesh` to an `Actor` after the last slot.
Result attach_mesh(World& world, Pool_Handle<Actor> handle, Pool_Handle<Mesh> mesh);
/// Attached a `mesh` to an `Actor` at `index`.
///
/// @note It resizes the array if `index` is out of bounds.
Result attach_mesh(World& world, Pool_Handle<Actor> handle, Pool_Handle<Mesh> mesh, size_t index);
/// Attaches a `mesh` to a `Prop` after the last slot.
Result attach_mesh(World& world, Pool_Handle<Prop> handle, Pool_Handle<Mesh> mesh);
/// Attached a `mesh` to a `Prop` at `index`.
///
/// @note It resizes the array if `index` is out of bounds.
Result attach_mesh(World& world, Pool_Handle<Prop> handle, Pool_Handle<Mesh> mesh, size_t index);
}  // namespace blk
