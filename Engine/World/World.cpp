// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/World/World.hpp"

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/World/Actor.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/Prop.hpp"

namespace
{
template <typename Type>
blk::Pool_Handle<Type> spawn_entity(blk::World& world, const char* name, blk::Pool_Handle<blk::Node> parent);

template <typename Type>
void despawn_entity(blk::World& world, blk::Pool_Handle<Type> handle);

template <typename Type>
blk::Pool_Handle<Type> find_entity(const blk::World& world, const char* name);

template <typename Type>
blk::Node* get_entity_node(const blk::World& world, blk::Pool_Handle<Type> handle);
}  // namespace

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
blk::despawn_node(World& world, Pool_Handle<Node> handle)
{
	std::vector<Pool_Handle<Node>> removed_node_handles = {};
	destroy_node(world.scene_graph, handle, removed_node_handles);

	for (const auto& removed_node_handle : removed_node_handles)
	{
		auto entity_instance_search = world.node_handle_to_entity_instance.find(removed_node_handle);

		if (entity_instance_search == world.node_handle_to_entity_instance.end())
		{
			continue;
		}

		switch (const Entity_Instance entity_instance = entity_instance_search->second; entity_instance.type)
		{
		case Entity_Type::Actor:
			world.actors.remove({.id = entity_instance.id, .version = entity_instance.version});
			break;
		case Entity_Type::Camera:
			world.cameras.remove({.id = entity_instance.id, .version = entity_instance.version});
			break;
		case Entity_Type::Prop:
			world.props.remove({.id = entity_instance.id, .version = entity_instance.version});
			break;
		}

		world.node_handle_to_entity_instance.erase(entity_instance_search);
	}
}

blk::Pool_Handle<blk::Actor>
blk::find_actor(const World& world, const char* name)
{
	return find_entity<Actor>(world, name);
}

blk::Pool_Handle<blk::Camera>
blk::find_camera(const World& world, const char* name)
{
	return find_entity<Camera>(world, name);
}

blk::Pool_Handle<blk::Prop>
blk::find_prop(const World& world, const char* name)
{
	return find_entity<Prop>(world, name);
}

blk::Actor*
blk::get_actor(const World& world, Pool_Handle<Actor> handle)
{
	return world.actors.get(handle);
}

blk::Camera*
blk::get_camera(const World& world, Pool_Handle<Camera> handle)
{
	return world.cameras.get(handle);
}

blk::Prop*
blk::get_prop(const World& world, Pool_Handle<Prop> handle)
{
	return world.props.get(handle);
}

blk::Node*
blk::get_entity_node(const World& world, Pool_Handle<Actor> handle)
{
	return ::get_entity_node(world, handle);
}

blk::Node*
blk::get_entity_node(const World& world, Pool_Handle<Camera> handle)
{
	return ::get_entity_node(world, handle);
}

blk::Node*
blk::get_entity_node(const World& world, Pool_Handle<Prop> handle)
{
	return ::get_entity_node(world, handle);
}

namespace
{
template <typename Type>
blk::Pool_Handle<Type>
spawn_entity(blk::World& world, const char* name, blk::Pool_Handle<blk::Node> parent)
{
	Type entity = {};
	entity.node_handle = create_node(world.scene_graph, name, parent);

	if (entity.node_handle == blk::POOL_HANDLE_NONE<blk::Node>)
	{
		return {};
	}

	blk::Pool_Handle<Type> entity_handle = {};
	blk::Entity_Instance entity_instance = {};

	if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		entity_handle = world.actors.insert(entity);
		entity_instance.type = blk::Entity_Type::Actor;
	}
	else if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		entity_handle = world.cameras.insert(entity);
		entity_instance.type = blk::Entity_Type::Camera;
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		entity_handle = world.props.insert(entity);
		entity_instance.type = blk::Entity_Type::Prop;
	}
	else
	{
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	entity_instance.id = entity_handle.id;
	entity_instance.version = entity_handle.version;

	world.node_handle_to_entity_instance.insert({entity.node_handle, entity_instance});

	return entity_handle;
}

template <typename Type>
void
despawn_entity(blk::World& world, blk::Pool_Handle<Type> handle)
{
	const Type* entity = nullptr;

	if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		entity = world.cameras.get(handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		entity = world.actors.get(handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		entity = world.props.get(handle);
	}
	else
	{
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (!entity)
	{
		return;
	}

	blk::despawn_node(world, entity->node_handle);
}

template <typename Type>
blk::Pool_Handle<Type>
find_entity(const blk::World& world, const char* name)
{
	auto node_handle_search = world.scene_graph.name_id_to_handle.find(blk::hash_fnv1a(name));

	if (node_handle_search == world.scene_graph.name_id_to_handle.end())
	{
		return {};
	}

	const blk::Pool_Handle<blk::Node> node_handle = node_handle_search->second;
	auto entity_instance_search = world.node_handle_to_entity_instance.find(node_handle);

	if (entity_instance_search == world.node_handle_to_entity_instance.end())
	{
		return {};
	}

	blk::Entity_Instance entity_instance = entity_instance_search->second;
	blk::Entity_Type expected_entity_type = {};

	if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		expected_entity_type = blk::Entity_Type::Camera;
	}
	else if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		expected_entity_type = blk::Entity_Type::Actor;
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		expected_entity_type = blk::Entity_Type::Prop;
	}
	else
	{
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (entity_instance.type != expected_entity_type)
	{
		return {};
	}

	return {.id = entity_instance.id, .version = entity_instance.version};
}

template <typename Type>
blk::Node*
get_entity_node(const blk::World& world, blk::Pool_Handle<Type> handle)
{
	Type* entity = nullptr;

	if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		entity = world.cameras.get(handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		entity = world.actors.get(handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		entity = world.props.get(handle);
	}
	else
	{
		static_assert(sizeof(Type) == 0, "Entity type not supported");
	}

	if (!entity)
	{
		return nullptr;
	}

	blk::Node* node = world.scene_graph.nodes.get(entity->node_handle);

	if (!node)
	{
		return nullptr;
	}

	return node;
}
}  // namespace
