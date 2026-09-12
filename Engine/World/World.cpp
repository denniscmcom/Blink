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
/// The following functions are the generic version of each `spawn_*`, `despawn`, `find_*`, `get_node`
/// functions exposed in the public API. They are only used internally to avoid code duplication. All requirements about
/// the parameters apply here too.
///
/// I decided to only expose the concrete functions because I find it cleaner.
/// @tparam Type Should be one of the entity types (e.g. `Actor`, `Camera`, ...).

template <typename Type>
blk::Pool_Handle<Type> spawn_entity(blk::World& world, const char* name, blk::Pool_Handle<blk::Node> parent);

template <typename Type>
void despawn_entity(blk::World& world, blk::Pool_Handle<Type> handle);

template <typename Type>
blk::Pool_Handle<Type> find_entity(blk::World& world, const char* name);

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
blk::despawn(World& world, Pool_Handle<Actor> handle)
{
	despawn_entity(world, handle);
}

void
blk::despawn(World& world, Pool_Handle<Camera> handle)
{
	despawn_entity(world, handle);
}

void
blk::despawn(World& world, Pool_Handle<Prop> handle)
{
	despawn_entity(world, handle);
}

void
blk::despawn(World& world, Pool_Handle<Node> handle)
{
	// First, we destroy the `Node` and all its subtree.

	Dyn_Array<Pool_Handle<Node>> destroyed_node_handles = {};

	if (create_dyn_array(destroyed_node_handles, world.allocator, 10) != Result::SUCCESS)
	{
		BLK_ERROR("Failed to despawn node\n");

		return;
	}

	destroy_node(world.scene_graph, handle, destroyed_node_handles);

	// Now, we update the state of	`world` to reflect the nodes destroyed.

	for (size_t i = 0; i < destroyed_node_handles.count; ++i)
	{
		const Pool_Handle<Node> destroyed_node_handle = destroyed_node_handles.buffer[i];

		// Remove the association from `world.node_handle_to_entity`.
		const Entity* entity = get(world.node_handle_to_entity, destroyed_node_handle);

		if (!BLK_VERIFY(entity))
		{
			// Association does not exists, so we continue. This should not really happen unless `world` is corrupted.
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
blk::get(World& world, Pool_Handle<Actor> handle)
{
	return get(world.actors, handle);
}

blk::Camera*
blk::get(World& world, Pool_Handle<Camera> handle)
{
	return get(world.cameras, handle);
}

blk::Prop*
blk::get(World& world, Pool_Handle<Prop> handle)
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

	if (const Result result = insert(node->mesh_instance.mesh_handles, mesh, index); result != Result::SUCCESS)
	{
		return result;
	}

	return Result::SUCCESS;
}

namespace
{
template <typename Type>
blk::Pool_Handle<Type>
spawn_entity(blk::World& world, const char* name, blk::Pool_Handle<blk::Node> parent)
{
	// First, we initialize a unique name for the node based on `name`.
	char unique_node_name[blk::MAX_NODE_NAME_SIZE];

	if (blk::init_unique_node_name(world.scene_graph, name, unique_node_name) != blk::Result::SUCCESS)
	{
		BLK_ERROR("Failed to create a unique name for entity\n");

		return {};
	}

	// Then, create the `Node` for the entity to spawn.

	Type entity = {};
	entity.node_handle = create_node(world.scene_graph, unique_node_name, parent);

	if (entity.node_handle == blk::POOL_HANDLE_NONE<blk::Node>)
	{
		return {};
	}

	// Depending on the entity type of `Type`, we assign the value for `generic_entity` and insert `entity` to the
	// correspondent `Pool` in `world`.

	blk::Pool_Handle<Type> entity_handle = {};
	blk::Entity generic_entity = {};

	if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		// `Type` is `Actor`.
		entity_handle = insert(world.actors, entity);
		generic_entity.type = blk::Entity_Type::ACTOR;
	}
	else if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		// `Type` is `Camera`.
		entity_handle = insert(world.cameras, entity);
		generic_entity.type = blk::Entity_Type::CAMERA;
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		// `Type` is `Prop`.
		entity_handle = insert(world.props, entity);
		generic_entity.type = blk::Entity_Type::PROP;
	}
	else
	{
		// Write more `else if` clauses when more `Entity_Type` are added.
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (entity_handle == blk::POOL_HANDLE_NONE<Type>)
	{
		BLK_ERROR("Failed to insert entity into its pool\n");

		// The node was created before the entity, so we destroy it here. It has no children and is not associated with
		// an `Entity` yet, so we cannot use `despawn`.
		blk::Dyn_Array<blk::Pool_Handle<blk::Node>> destroyed_node_handles = {};

		if (create_dyn_array(destroyed_node_handles, world.allocator, 1) == blk::Result::SUCCESS)
		{
			destroy_node(world.scene_graph, entity.node_handle, destroyed_node_handles);
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

	blk::despawn(world, entity->node_handle);
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
