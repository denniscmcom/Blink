// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/World/World.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Core/String.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/Actor.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/Prop.hpp"
#include "Engine/World/Terrain.hpp"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <type_traits>

namespace
{
/// Spawns a node and attachs an entity `Type` to it.
/// Returns none on failure.
template <typename Type>
blk::Pool_Handle<Type> spawn_entity(blk::World& world, blk::Pool_Handle<blk::Node> parent_handle);

/// Despawns an entity and its subtree from `world`.
template <typename Type>
void despawn_entity(blk::World& world, blk::Pool_Handle<Type> handle);

/// Removes the entity attached to `node_handle`, if any, from its pool and from `world.node_handle_to_entity`.
/// It does not despawn the node.
void despawn_attached_entity(blk::World& world, blk::Pool_Handle<blk::Node> node_handle);

/// Updates `lod` for a terrain chunk `distance` chunks from the focus chunk, using `Terrain_Settings::lod_radii` and
/// `Terrain_Settings::lod_hysteresis`.
void update_terrain_lod(const blk::Terrain_Settings& settings, int32_t distance, uint32_t& lod);
}  // namespace

blk::Result
blk::create_world(Allocator* allocator, const World_Settings& settings, World& world)
{
	if (!BLK_VERIFY(allocator) || !BLK_VERIFY(settings.terrain.chunk_texel_count > 0) ||
		!BLK_VERIFY(settings.stream.unload_radius > settings.stream.load_radius) ||
		!BLK_VERIFY(settings.stream.rebase_distance > 0) || !BLK_VERIFY(settings.stream.max_loads_per_update > 0))
	{
		return Result::INVALID_ARGUMENTS;
	}

	for (size_t i = 1; i < TERRAIN_LOD_COUNT - 1; ++i)
	{
		if (!BLK_VERIFY(settings.terrain.lod_radii.buffer[i] > settings.terrain.lod_radii.buffer[i - 1]))
		{
			return Result::INVALID_ARGUMENTS;
		}
	}

	if (!BLK_VERIFY(settings.terrain.lod_hysteresis >= 0))
	{
		return Result::INVALID_ARGUMENTS;
	}

	world = {};
	world.allocator = allocator;
	world.settings = settings;

	// Create pools.

	if (const Result result = create_pool(world.actors, allocator, 128); result != Result::SUCCESS)
	{
		return result;
	}

	if (const Result result = create_pool(world.cameras, allocator, 4); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	if (const Result result = create_pool(world.props, allocator, 128); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	// Create terrain grids, one per LOD.
	// A small `chunk_texel_count` runs out of quads before the last LOD, so those LODs share the single quad grid.

	for (size_t i = 0; i < TERRAIN_LOD_COUNT; ++i)
	{
		const uint32_t quad_count = settings.terrain.chunk_texel_count >> i;

		Pool_Handle<Mesh>& grid_handle = world.settings.terrain.chunk_grid_handles.buffer[i];
		grid_handle = compute_grid(quad_count > 0 ? quad_count : 1);

		if (grid_handle == POOL_HANDLE_NONE<Mesh>)
		{
			BLK_ERROR("Failed to compute terrain grid LOD%zu\n", i);
			destroy_world(world);

			return Result::INVALID_ARGUMENTS;
		}
	}

	// Create hash map.

	if (const Result result = create_hash_map(world.node_handle_to_entity, allocator, 256, 0.75f);
		result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	// Create despawned node array.

	if (const Result result = create_dyn_array(world.despawned_nodes, allocator, 256); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	// Create chunk array.

	// Chunks stay loaded until they are farther than `unload_radius` from the focus on either axis, so at most a square
	// of `2 * unload_radius + 1` chunks per side is loaded. If the radius is raised later, the array grows.
	const size_t chunk_side_count = static_cast<size_t>(settings.stream.unload_radius) * 2 + 1;
	const size_t chunk_count = chunk_side_count * chunk_side_count;

	if (const Result result = create_dyn_array(world.chunks, allocator, chunk_count); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	// Create scene graph.

	if (const Result result = create_scene_graph(world.scene_graph, allocator); result != Result::SUCCESS)
	{
		destroy_world(world);

		return result;
	}

	// Spawn environment node.

	const Pool_Handle<Node> environment_handle = spawn_node(world.scene_graph, Node_Type::SPATIAL, {});

	if (environment_handle == POOL_HANDLE_NONE<Node>)
	{
		// TODO (Bug): returns without `destroy_world`, and the result does not describe the failure.
		BLK_ERROR("Failed to spawn environment node\n");

		return Result::INVALID_ARGUMENTS;
	}

	BLK_IF_NOT_SUCCESS(rename_node(world.scene_graph, environment_handle, "Environment"))
	{
		BLK_ERROR("Failed to rename environment node\n");
	}

	// Spawn sun node.

	const Pool_Handle<Node> sun_handle =
		spawn_node(world.scene_graph, Node_Type::DIRECTIONAL_LIGHT, environment_handle);

	if (sun_handle == POOL_HANDLE_NONE<Node>)
	{
		// TODO (Bug): returns without `destroy_world`, and the result does not describe the failure.
		BLK_ERROR("Failed to spawn sun node\n");

		return Result::INVALID_ARGUMENTS;
	}

	BLK_IF_NOT_SUCCESS(rename_node(world.scene_graph, sun_handle, "Sun"))
	{
		BLK_ERROR("Failed to rename sun node\n");
	}

	// TODO (Bug): `sun` is not checked for null.
	Node* sun = get_node(world.scene_graph, sun_handle);

	// TODO (Bug): these look like degrees, but `Transform::rotation` is in radians.
	sun->transform.rotation.x = 40.0f;
	sun->transform.rotation.y = 90.0f;

	world.settings.atmosphere.sun_node_handle = sun_handle;

	return Result::SUCCESS;
}

void
blk::destroy_world(World& world)
{
	// Destroy pools.

	destroy_pool(world.actors);
	destroy_pool(world.cameras);
	destroy_pool(world.props);

	// Destroy hash map.
	destroy_hash_map(world.node_handle_to_entity);

	// Destroy scene graph.
	destroy_scene_graph(world.scene_graph);

	// Destroy dynamic array.
	destroy_dyn_array(world.despawned_nodes);
	destroy_dyn_array(world.chunks);

	world = {};
}

void
blk::update_world(World& world, Vector3& node_offset)
{
	node_offset = {};

	// Handle and clear flags.

	if (world.flags & WORLD_TERRAIN_DIRTY_BIT)
	{
		despawn_chunks(world);
	}

	world.flags = 0;

	// Get active camera node.

	const Camera* camera = get_camera(world, world.active_camera_handle);

	if (!camera)
	{
		// Without a camera we have nothing to update.
		return;
	}

	const Node* camera_node = get_node(world.scene_graph, camera->node_handle);

	if (!BLK_VERIFY(camera_node))
	{
		// This should not happen.
		return;
	}

	// Compute which chunk the focus position is in, relative to the origin.
	// The camera can be parented to any node, so we use its world position. `update_node_transforms` runs after this
	// function, so it lags one frame, which does not matter for streaming.

	float chunk_size = compute_chunk_size(world.settings.terrain);

	if (chunk_size <= 0.0f)
	{
		// Without a configured terrain we have no chunks to stream.
		return;
	}

	Chunk_Coord local = {};
	local.x = static_cast<int32_t>(floorf(camera_node->world_matrix.columns[3].x / chunk_size));
	local.z = static_cast<int32_t>(floorf(camera_node->world_matrix.columns[3].z / chunk_size));

	// Rebase origin if needed.

	const Node* root = get_node(world.scene_graph, world.scene_graph.root);

	if (!BLK_VERIFY(root))
	{
		// This should not happen.
		return;
	}

	if (abs(local.x) >= world.settings.stream.rebase_distance || abs(local.z) >= world.settings.stream.rebase_distance)
	{
		BLK_DEBUG("Rebasing origin: %i, %i\n", local.x, local.z);

		// Move `world.origin` to `local`.

		world.origin.x += local.x;
		world.origin.z += local.z;

		// Every node under root moves the other way by the same distance, so nothing changes visually.
		// Node children follow their root.

		node_offset.x = -static_cast<float>(local.x) * chunk_size;
		node_offset.z = -static_cast<float>(local.z) * chunk_size;

		Pool_Handle<Node> child_handle = root->first_child_handle;

		while (Node* child = get_node(world.scene_graph, child_handle))
		{
			child->transform.position.x += node_offset.x;
			child->transform.position.z += node_offset.z;

			child_handle = child->next_sibling_handle;
		}

		// `local` is now at origin (0, 0).
		local = {};
	}

	// Convert the focus chunk from "relative to origin" to its absolute coordinate in the grid world.

	Chunk_Coord absolute = {};
	absolute.x = world.origin.x + local.x;
	absolute.z = world.origin.z + local.z;

	// Unload chunks that are too far.
	// Instead of removing a chunk from the middle of the array and shifting everything down, we use swap-remove so that
	// removing that element is O(1).

	size_t unload_counter = 0;

	while (unload_counter < world.chunks.count)
	{
		Chunk& chunk = world.chunks.buffer[unload_counter];

		// TODO (Bug): A chunk whose root is despawned from elsewhere keeps its entry here with a stale `root_handle`,
		// so the load loop treats its coordinate as loaded and it never respawns while in range. Also drop chunks whose
		// root no longer exists.

		// Check if the chunk is outside of bounds.

		const int32_t dx = abs(chunk.coord.x - absolute.x);
		const int32_t dz = abs(chunk.coord.z - absolute.z);

		if (dx > world.settings.stream.unload_radius || dz > world.settings.stream.unload_radius)
		{
			// Chunk is outside the loaded radius.
			// Despawn chunk and delete it from the array.

			despawn_subtree(world, chunk.root_handle);
			chunk = world.chunks.buffer[world.chunks.count - 1];
			world.chunks.count -= 1;
		}
		else
		{
			// Chunk is within the loaded radius.
			// The focus may have moved since it was loaded, so we update its terrain LOD.

			if (Node* terrain = get_node(world.scene_graph, chunk.terrain_handle))
			{
				update_terrain_lod(world.settings.terrain, dx > dz ? dx : dz, terrain->terrain_ref.lod);
			}

			// We keep checking the next chunks.
			unload_counter += 1;
		}
	}

	// Load missing chunks in square rings.
	// We go ring by ring, loading nearest chunks first.

	// Counter of chunks loaded to compare against `settings.max_loads_per_update`.
	int32_t chunks_loaded_count = 0;

	// TODO (Performance): Keep in mind that these three nested loops may cause problems.

	for (int32_t ring = 0; ring <= world.settings.stream.load_radius; ++ring)
	{
		for (int32_t dz = -ring; dz <= ring; ++dz)
		{
			for (int32_t dx = -ring; dx <= ring; ++dx)
			{
				if (abs(dx) != ring && abs(dz) != ring)
				{
					// Previous iterations handled chunks in smaller rings.
					continue;
				}

				Chunk_Coord coord = {};
				coord.x = absolute.x + dx;
				coord.z = absolute.z + dz;

				// Skip chunks already loaded by previous frames.
				bool is_loaded = false;

				for (size_t i = 0; i < world.chunks.count; ++i)
				{
					if (const Chunk_Coord loaded_coord = world.chunks.buffer[i].coord;
						loaded_coord.x == coord.x && loaded_coord.z == coord.z)
					{
						is_loaded = true;
						break;
					}
				}

				if (is_loaded)
				{
					continue;
				}

				// Load chunk.

				// Spawn the chunk root.

				const Pool_Handle<Node> chunk_handle = spawn_node(world.scene_graph, Node_Type::SPATIAL, {});

				if (chunk_handle == POOL_HANDLE_NONE<Node>)
				{
					BLK_ERROR("Failed to spawn chunk root\n");
					continue;
				}

				// Place the root relative to the current origin. Its children are relative to it.

				Node* chunk_node = get_node(world.scene_graph, chunk_handle);

				if (!BLK_VERIFY(chunk_node))
				{
					// This should not happen.
					despawn_subtree(world, chunk_handle);
					continue;
				}

				chunk_node->transform.position.x = static_cast<float>(coord.x - world.origin.x) * chunk_size;
				chunk_node->transform.position.z = static_cast<float>(coord.z - world.origin.z) * chunk_size;

				// Rename the node.

				char chunk_name[MAX_NODE_NAME_SIZE] = {};

				BLK_IF_NOT_SNPRINTF(chunk_name, MAX_NODE_NAME_SIZE, "Chunk_%d_%d", coord.x, coord.z)
				{
					BLK_ERROR("Failed to format chunk node name\n");
				}
				else
				{
					BLK_IF_NOT_SUCCESS(rename_node(world.scene_graph, chunk_handle, chunk_name))
					{
						BLK_ERROR("Failed to rename chunk node\n");
					}
				}

				// Generate terrain.

				const uint32_t chunk_count =
					world.settings.terrain.resolution / world.settings.terrain.chunk_texel_count;

				const bool is_inside_terrain = coord.x >= 0 && coord.z >= 0 &&
											   coord.x < static_cast<int32_t>(chunk_count) &&
											   coord.z < static_cast<int32_t>(chunk_count);

				Pool_Handle<Node> terrain_handle = {};

				if (is_inside_terrain)
				{
					// The chunk root sits at the chunk's corner, so the terrain node stays at its origin.

					terrain_handle = spawn_node(world.scene_graph, Node_Type::TERRAIN, chunk_handle);
					Node* terrain = get_node(world.scene_graph, terrain_handle);

					if (!terrain)
					{
						BLK_ERROR("Failed to spawn the terrain node of chunk %d, %d\n", coord.x, coord.z);
					}
					else
					{
						BLK_IF_NOT_SUCCESS(rename_node(world.scene_graph, terrain_handle, "Terrain"))
						{
							BLK_ERROR("Failed to rename the terrain node\n");
						}

						// Each chunk covers its own square of the heightmap.

						const float uv_scale = 1.0f / static_cast<float>(chunk_count);

						terrain->terrain_ref.uv_offset = Vector2{
							.x = static_cast<float>(coord.x) * uv_scale,
							.y = static_cast<float>(coord.z) * uv_scale,
						};

						terrain->terrain_ref.uv_scale = uv_scale;

						// Starting from the coarsest LOD, the update only moves finer, so the chunk gets the LOD of
						// its distance without hysteresis. `ring` is that distance.
						terrain->terrain_ref.lod = static_cast<uint32_t>(TERRAIN_LOD_COUNT - 1);
						update_terrain_lod(world.settings.terrain, ring, terrain->terrain_ref.lod);
					}
				}

				// Call the generation function for the current chunk.

				if (world.settings.stream.generate)
				{
					world.settings.stream.generate(world, chunk_handle, coord);
				}

				// Push node to loaded list.

				Chunk chunk = {};
				chunk.coord = coord;
				chunk.root_handle = chunk_handle;
				chunk.terrain_handle = terrain_handle;

				push(world.chunks, chunk);
				chunks_loaded_count += 1;

				// We skip loading more chunks this frame.

				if (chunks_loaded_count >= world.settings.stream.max_loads_per_update)
				{
					return;
				}
			}
		}
	}
}

blk::Pool_Handle<blk::Actor>
blk::spawn_actor(World& world, Pool_Handle<Node> parent_handle)
{
	return spawn_entity<Actor>(world, parent_handle);
}

blk::Pool_Handle<blk::Camera>
blk::spawn_camera(World& world, Pool_Handle<Node> parent_handle)
{
	return spawn_entity<Camera>(world, parent_handle);
}

blk::Pool_Handle<blk::Prop>
blk::spawn_prop(World& world, Pool_Handle<Node> parent_handle)
{
	return spawn_entity<Prop>(world, parent_handle);
}

void
blk::despawn_node(World& world, Pool_Handle<Node> handle)
{
	// Only this node goes away, so only its own entity does. Its children keep theirs.
	despawn_attached_entity(world, handle);
	despawn_node(world.scene_graph, handle);
}

void
blk::despawn_subtree(World& world, Pool_Handle<Node> handle)
{
	// Clean up reused array.
	empty(world.despawned_nodes);

	// Despawn the subtree.
	despawn_subtree(world.scene_graph, handle, &world.despawned_nodes);

	// For every despawned node, we despawn its entity from `world`.

	for (size_t i = 0; i < world.despawned_nodes.count; ++i)
	{
		despawn_attached_entity(world, world.despawned_nodes.buffer[i]);
	}
}

void
blk::despawn_chunks(World& world)
{
	for (size_t i = 0; i < world.chunks.count; ++i)
	{
		despawn_subtree(world, world.chunks.buffer[i].root_handle);
	}

	empty(world.chunks);
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

namespace
{
template <typename Type>
blk::Pool_Handle<Type>
spawn_entity(blk::World& world, blk::Pool_Handle<blk::Node> parent_handle)
{
	// Spawns spatial node.

	const blk::Pool_Handle<blk::Node> node_handle =
		spawn_node(world.scene_graph, blk::Node_Type::SPATIAL, parent_handle);

	if (!BLK_VERIFY(node_handle != blk::POOL_HANDLE_NONE<blk::Node>))
	{
		return {};
	}

	// Assign the entity's node.

	Type entity = {};
	entity.node_handle = node_handle;

	// Insert entity into the pool and assign entity type.

	blk::Pool_Handle<Type> entity_handle = {};
	blk::Entity_Ref entity_ref = {};

	if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		entity_handle = insert(world.actors, entity);
		entity_ref.type = blk::Entity_Type::ACTOR;
	}
	else if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		entity_handle = insert(world.cameras, entity);
		entity_ref.type = blk::Entity_Type::CAMERA;
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		entity_handle = insert(world.props, entity);
		entity_ref.type = blk::Entity_Type::PROP;
	}
	else
	{
		static_assert(sizeof(Type) == 0, "Entity type not supported\n");
	}

	if (!BLK_VERIFY(entity_handle != blk::POOL_HANDLE_NONE<Type>))
	{
		despawn_subtree(world.scene_graph, node_handle, nullptr);

		return {};
	}

	// Assign entity handle to entity ref.

	entity_ref.id = entity_handle.id;
	entity_ref.version = entity_handle.version;

	// Insert node-entity association.
	insert(world.node_handle_to_entity, node_handle, entity_ref);

	return entity_handle;
}

template <typename Type>
void
despawn_entity(blk::World& world, blk::Pool_Handle<Type> handle)
{
	// Get entity.

	const Type* entity = nullptr;

	if constexpr (std::is_same_v<Type, blk::Actor>)
	{
		entity = get(world.actors, handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Camera>)
	{
		entity = get(world.cameras, handle);
	}
	else if constexpr (std::is_same_v<Type, blk::Prop>)
	{
		entity = get(world.props, handle);
	}
	else
	{
		static_assert(sizeof(Type) == 0, "Entity type not supported\n");
	}

	if (!entity)
	{
		// Nothing to despawn.
		return;
	}

	// The entity is attached to its node, so despawning the node's subtree despawns it along with every entity below.
	blk::despawn_subtree(world, entity->node_handle);
}

void
despawn_attached_entity(blk::World& world, blk::Pool_Handle<blk::Node> node_handle)
{
	const blk::Entity_Ref* entity_ref = get(world.node_handle_to_entity, node_handle);

	if (!entity_ref)
	{
		// A plain node with nothing attached.
		return;
	}

	// Despawn the entity from its pool.

	switch (entity_ref->type)
	{
	case blk::Entity_Type::ACTOR: {
		remove(world.actors, {.id = entity_ref->id, .version = entity_ref->version});
	}
	break;
	case blk::Entity_Type::CAMERA: {
		const blk::Pool_Handle<blk::Camera> camera_handle = {
			.id = entity_ref->id,
			.version = entity_ref->version,
		};

		if (world.active_camera_handle == camera_handle)
		{
			// Clean up world active camera.
			world.active_camera_handle = {};
		}

		remove(world.cameras, camera_handle);
	}
	break;
	case blk::Entity_Type::PROP: {
		remove(world.props, {.id = entity_ref->id, .version = entity_ref->version});
	}
	break;
	case blk::Entity_Type::NONE:
		break;
	}

	// Remove the node-entity ref association.
	remove(world.node_handle_to_entity, node_handle);
}

void
update_terrain_lod(const blk::Terrain_Settings& settings, const int32_t distance, uint32_t& lod)
{
	// Move to a finer LOD as soon as the chunk is within its radius.
	while (lod > 0 && distance <= settings.lod_radii.buffer[lod - 1])
	{
		lod -= 1;
	}

	// Move to a coarser LOD only once the chunk is past its radius plus the hysteresis.
	while (lod < blk::TERRAIN_LOD_COUNT - 1 && distance > settings.lod_radii.buffer[lod] + settings.lod_hysteresis)
	{
		lod += 1;
	}
}
}  // namespace
