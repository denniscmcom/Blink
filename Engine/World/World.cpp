// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/World/World.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/Actor.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/Prop.hpp"

#include <type_traits>

namespace
{
/// The following functions are the generic version of each `spawn_*`, `despawn_*`, `find_*`, `get_node`
/// functions exposed in the public API. They are only used internally to avoid code duplication. All requirements about
/// the parameters apply here too.

/// Spawns a entity into world.
/// @tparam Type Should be one of the entity types (e.g. `Actor`, `Camera`, ...).
template <typename Type>
blk::Pool_Handle<Type> spawn_entity(blk::World& world, const char* name, blk::Pool_Handle<blk::Node> parent);

/// Despawns an entity from `world`.
/// @tparam Type Should be one of the entity types (e.g. `Actor`, `Camera`, ...).
template <typename Type>
void despawn_entity(blk::World& world, blk::Pool_Handle<Type> handle);

/// Finds an entity in `world` by `name` and returns its handle.
/// If it does not exists, returns `POOL_HANDLE_NODE<Type>`.
/// @tparam Type Should be one of the entity types (e.g. `Actor`, `Camera`, ...).
template <typename Type>
blk::Pool_Handle<Type> find_entity(blk::World& world, const char* name);

/// Gets the `Node` of an entity by its `handle` and returns a pointer to it.
/// If it does not exists, returns a `nullptr`.
/// @tparam Type Should be one of the entity types (e.g. `Actor`, `Camera`, ...).
template <typename Type>
blk::Node* get_entity_node(blk::World& world, blk::Pool_Handle<Type> handle);
}  // namespace

blk::Result
blk::create_world(Allocator* allocator, World& world)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	world = {};
	world.allocator = allocator;

	// A failure leaves everything created so far without an owner, so we tear the whole `world` down before returning.

	if (const Result result = create_pool(world.actors, allocator, 100); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	if (const Result result = create_pool(world.cameras, allocator, 3); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	if (const Result result = create_pool(world.props, allocator, 100); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	if (const Result result = create_scene_graph(world.scene_graph, allocator); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	if (const Result result = create_hash_map(world.node_handle_to_entity, allocator, 100, 0.75f);
		result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	return Result::SUCCESS;
}

void
blk::destroy_world(World& world)
{
	destroy_pool(world.actors);
	destroy_pool(world.cameras);
	destroy_pool(world.props);
	destroy_scene_graph(world.scene_graph);
	destroy_hash_map(world.node_handle_to_entity);

	world = {};
}

blk::Pool_Handle<blk::Actor>
blk::spawn_actor(World& world, const char* name, Pool_Handle<Node> parent)
{
	return spawn_entity<Actor>(world, name, parent);
}

blk::Pool_Handle<blk::Camera>
blk::spawn_camera(World& world, const char* name, Pool_Handle<Node> parent)
{
	return spawn_entity<Camera>(world, name, parent);
}

blk::Pool_Handle<blk::Prop>
blk::spawn_prop(World& world, const char* name, Pool_Handle<Node> parent)
{
	return spawn_entity<Prop>(world, name, parent);
}

void
blk::despawn_actor(World& world, Pool_Handle<Actor> handle)
{
	despawn_entity(world, handle);
}

void
blk::despawn_camera(World& world, Pool_Handle<Camera> handle)
{
	despawn_entity(world, handle);
}

void
blk::despawn_prop(World& world, Pool_Handle<Prop> handle)
{
	despawn_entity(world, handle);
}

void
blk::despawn_entity_by_node(World& world, Pool_Handle<Node> handle)
{
	// First, we destroy the `Node` and all its subtree.

	Dyn_Array<Pool_Handle<Node>> destroyed_node_handles = {};

	if (create_dyn_array(destroyed_node_handles, world.allocator, 10) != Result::SUCCESS)
	{
		BLK_ERROR("Failed to despawn node\n");

		return;
	}

	despawn_node(world.scene_graph, handle, destroyed_node_handles);

	// Now, we update the state of	`world` to reflect the nodes destroyed.

	for (size_t i = 0; i < destroyed_node_handles.count; ++i)
	{
		const Pool_Handle<Node> destroyed_node_handle = destroyed_node_handles.buffer[i];

		// Remove the association from `world.node_handle_to_entity`.
		const Entity* entity = get(world.node_handle_to_entity, destroyed_node_handle);

		if (!entity)
		{
			// Association does not exists, so we continue. This could happen with certain node types like
			// `POINT_LIGHT`.
			continue;
		}

		// Then, we remove the concrete entity from its correspondent pool.

		switch (entity->type)
		{
		case Entity_Type::ACTOR: {
			const Pool_Handle<Actor> actor_handle = {.id = entity->id, .version = entity->version};
			remove(world.actors, actor_handle);
		}
		break;
		case Entity_Type::CAMERA: {
			const Pool_Handle<Camera> camera_handle = {.id = entity->id, .version = entity->version};

			if (world.active_camera_handle == camera_handle)
			{
				// We are despawning the active camera, so `world` is left without one.
				world.active_camera_handle = {};
			}

			remove(world.cameras, camera_handle);
		}
		break;
		case Entity_Type::PROP: {
			const Pool_Handle<Prop> prop_handle = {.id = entity->id, .version = entity->version};
			remove(world.props, prop_handle);
		}
		break;
		}

		// Finally, we remove the association from `world.node_handle_to_entity`.
		remove(world.node_handle_to_entity, destroyed_node_handle);
	}

	destroy_dyn_array(destroyed_node_handles);
}

blk::Pool_Handle<blk::Actor>
blk::find_actor(World& world, const char* name)
{
	return find_entity<Actor>(world, name);
}

blk::Pool_Handle<blk::Camera>
blk::find_camera(World& world, const char* name)
{
	return find_entity<Camera>(world, name);
}

blk::Pool_Handle<blk::Prop>
blk::find_prop(World& world, const char* name)
{
	return find_entity<Prop>(world, name);
}

blk::Actor*
blk::get_actor(World& world, Pool_Handle<Actor> handle)
{
	return get(world.actors, handle);
}

blk::Camera*
blk::get_camera(World& world, Pool_Handle<Camera> handle)
{
	return get(world.cameras, handle);
}

blk::Prop*
blk::get_prop(World& world, Pool_Handle<Prop> handle)
{
	return get(world.props, handle);
}

blk::Node*
blk::get_node(World& world, Pool_Handle<Actor> handle)
{
	return get_entity_node(world, handle);
}

blk::Node*
blk::get_node(World& world, Pool_Handle<Camera> handle)
{
	return get_entity_node(world, handle);
}

blk::Node*
blk::get_node(World& world, Pool_Handle<Prop> handle)
{
	return get_entity_node(world, handle);
}

blk::Result
blk::attach_mesh(World& /*world*/, Pool_Handle<Actor> /*handle*/, Pool_Handle<Mesh> /*mesh*/)
{
	// TODO (Feature): not implemented.
	BLK_NOT_IMPLEMENTED();

	return Result::SUCCESS;
}

blk::Result
blk::attach_mesh(World& /*world*/, Pool_Handle<Actor> /*handle*/, Pool_Handle<Mesh> /*mesh*/, size_t /*index*/)
{
	// TODO (Feature): not implemented.
	BLK_NOT_IMPLEMENTED();

	return Result::SUCCESS;
}

blk::Result
blk::attach_mesh(World& world, Pool_Handle<Prop> handle, Pool_Handle<Mesh> mesh)
{
	Node* node = get_node(world, handle);

	if (!node)
	{
		BLK_ERROR("Failed to node for prop\n");

		return Result::INVALID_ARGUMENTS;
	}

	push(node->mesh_instance.mesh_handles, mesh);

	return Result::SUCCESS;
}

blk::Result
blk::attach_mesh(World& world, Pool_Handle<Prop> handle, Pool_Handle<Mesh> mesh, size_t index)
{
	Node* node = get_node(world, handle);

	if (!node)
	{
		BLK_ERROR("Failed to node for prop\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Verify if `index` is out of bounds.

	if (node->mesh_instance.mesh_handles.capacity < index)
	{
		// `index` starts at `0`, so we need a capacity of 1 at least.
		BLK_SUCCESS_OR_ERROR_RETURN(
			resize(node->mesh_instance.mesh_handles, index + 1),
			"Failed to resize mesh handles array\n"
		);
	}

	BLK_SUCCESS_OR_RETURN(set(node->mesh_instance.mesh_handles, mesh, index));

	return Result::SUCCESS;
}

namespace
{
template <typename Type>
blk::Pool_Handle<Type>
spawn_entity(blk::World& world, const char* name, blk::Pool_Handle<blk::Node> parent)
{
	// Depending on the entity type of `Type`, we assign the value for `generic_entity` and insert `entity` to the
	// correspondent `Pool` in `world`.

	Type entity = {};
	blk::Pool_Handle<Type> entity_handle = {};
	blk::Entity generic_entity = {};

	if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		// `Type` is `Actor`.
		entity.node_handle = spawn_node(world.scene_graph, name, blk::Node_Type::MESH_INSTANCE, parent);
		entity_handle = insert(world.actors, entity);
		generic_entity.type = blk::Entity_Type::ACTOR;
	}
	else if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		// `Type` is `Camera`.
		entity.node_handle = spawn_node(world.scene_graph, name, blk::Node_Type::SPATIAL, parent);
		entity_handle = insert(world.cameras, entity);
		generic_entity.type = blk::Entity_Type::CAMERA;
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		// `Type` is `Prop`.
		entity.node_handle = spawn_node(world.scene_graph, name, blk::Node_Type::MESH_INSTANCE, parent);
		entity_handle = insert(world.props, entity);
		generic_entity.type = blk::Entity_Type::PROP;
	}
	else
	{
		// Write more `else if` clauses when more `Entity_Type` are added.
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (entity.node_handle == blk::POOL_HANDLE_NONE<blk::Node>)
	{
		BLK_ERROR("Failed to spawn entity node\n");

		return {};
	}

	if (entity_handle == blk::POOL_HANDLE_NONE<Type>)
	{
		BLK_ERROR("Failed to insert entity into its pool\n");

		// The node was created before the entity, so we destroy it here. It has no children and is not associated with
		// an `Entity` yet, so we cannot use `despawn_*`.
		blk::Dyn_Array<blk::Pool_Handle<blk::Node>> destroyed_node_handles = {};

		if (create_dyn_array(destroyed_node_handles, world.allocator, 1) == blk::Result::SUCCESS)
		{
			despawn_node(world.scene_graph, entity.node_handle, destroyed_node_handles);
			destroy_dyn_array(destroyed_node_handles);
		}

		return {};
	}

	// Assign the `Pool_Handle` obtained from inserting `entity` into its pool to the fields to `Entity`.
	generic_entity.id = entity_handle.id;
	generic_entity.version = entity_handle.version;

	// Finally, we insert `node_handle-entity` pair into the `Hash_Map`.
	insert(world.node_handle_to_entity, entity.node_handle, generic_entity);

	return entity_handle;
}

template <typename Type>
void
despawn_entity(blk::World& world, blk::Pool_Handle<Type> handle)
{
	const Type* entity = nullptr;

	// Depending on `Type`, we get the entity from its correspondent pool.

	if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		// `Type` is `Camera`.
		entity = get(world.cameras, handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		// `Type` is `Actor`.
		entity = get(world.actors, handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		// `Type` is `Prop`.
		entity = get(world.props, handle);
	}
	else
	{
		// Write more `else if` clauses when more `Entity_Type` are added.
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (!entity)
	{
		// Entity does not exists in the pool, so there is nothing to despawn.
		return;
	}

	blk::despawn_entity_by_node(world, entity->node_handle);
}

template <typename Type>
blk::Pool_Handle<Type>
find_entity(blk::World& world, const char* name)
{
	if (!BLK_VERIFY(name))
	{
		return {};
	}

	// TODO (Consistency): This reaches into `Scene_Graph::hash_to_handle`, so `World` depends on how `Scene_Graph`
	// stores node names. `find_node` cannot be used because it returns a `Node*` and we need the handle. Add a
	// `find_node_handle` to `Scene/Graph.hpp` and call it here instead.

	// Get `Pool_Handle<Type>` from `world.scene_graph` given its hash.
	const uint64_t hash = blk::hash_fnv1a(name);
	const blk::Pool_Handle<blk::Node>* node_handle = blk::get(world.scene_graph.hash_to_handle, hash);

	if (!node_handle)
	{
		// Node does not exists with `name`.
		return {};
	}

	// Get `Entity` associated with `node_handle`.
	const blk::Entity* entity = get(world.node_handle_to_entity, *node_handle);

	if (!entity)
	{
		// Node is not an `Entity`. See `find_node`.
		return {};
	}

	// Now, we check if `entity->type` is the same as `Type`. This should be always the case at least something is wrong
	// with the implementation.

	blk::Entity_Type expected_entity_type = {};

	if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		expected_entity_type = blk::Entity_Type::CAMERA;
	}
	else if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		expected_entity_type = blk::Entity_Type::ACTOR;
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		expected_entity_type = blk::Entity_Type::PROP;
	}
	else
	{
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (!BLK_VERIFY(entity->type == expected_entity_type))
	{
		return {};
	}

	return blk::Pool_Handle<Type>{
		.id = entity->id,
		.version = entity->version,
	};
}

template <typename Type>
blk::Node*
get_entity_node(blk::World& world, blk::Pool_Handle<Type> handle)
{
	// Get the entity from its correspondent pool in `world`.

	const Type* entity = nullptr;

	if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		// `Type` is `Camera`.
		entity = get(world.cameras, handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		// `Type` is `Actor`.
		entity = get(world.actors, handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		// `Type` is `Prop`.
		entity = get(world.props, handle);
	}
	else
	{
		// Write more `else if` clauses when adding more `Entity_Type`.
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (!entity)
	{
		// Entity does not exists in pool.
		return nullptr;
	}

	blk::Node* node = get(world.scene_graph.nodes, entity->node_handle);

	if (!BLK_VERIFY(node))
	{
		// This should not happen because every entity in `world` has a `Node` in `world.scene_graph`. If this happens,
		// it means `world` is corrupted.
		return nullptr;
	}

	return node;
}
}  // namespace
