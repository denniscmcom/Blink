// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Mesh.hpp"

#include "Engine/Core/Math/Constants.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Storage.hpp"

#include <math.h>

namespace
{
blk::Resource_Storage<blk::Mesh> storage = {};
}  // namespace

blk::Result
blk::create_mesh_storage(Allocator* allocator)
{
	return create_resource_storage(storage, allocator);
}

void
blk::destroy_mesh_storage()
{
	// TODO (Bug): We are leaking `Mesh::vertices` and `Mesh::indices`.
	// They are dynamic arrays created by `load_mesh`, so we should destroy them before destroying `storage`.
	// I have not fixed this yet because it is not really that important. We only destroy `storage` on application exit,
	// so in reality we are not leaking anything.
	destroy_resource_storage(storage);
}

blk::Pool_Handle<blk::Mesh>
blk::load_mesh(const char* stem)
{
	if (!BLK_VERIFY(stem))
	{
		return {};
	}

	// Compute the mesh hash.
	const uint64_t hash = get_resource_hash(stem, Resource_Type::MESH);

	if (is_resource_loaded(storage, hash))
	{
		// If the mesh is already loaded, we return its handle.
		return get_resource_handle(storage, hash);
	}

	Serial serial = {};

	// `deserialize_resource` allocates `serial.buffer` and handle ownership to us, so we need to free it.
	BLK_IF_NOT_SUCCESS(deserialize_resource(storage.pool.allocator, Resource_Type::MESH, hash, serial))
	{
		BLK_ERROR("Failed to deserialize mesh `%llu`\n", hash);

		return {};
	}

	// Read vertex count.
	uint32_t vertex_count = 0;

	BLK_IF_NOT_SUCCESS(read(serial, vertex_count))
	{
		BLK_ERROR("Failed to read vertex count\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	// Read index count.
	uint32_t index_count = 0;

	BLK_IF_NOT_SUCCESS(read(serial, index_count))
	{
		BLK_ERROR("Failed to read index count\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	Mesh mesh = {};

	// Allocate `mesh.vertices` with capacity `vertex_count`.

	BLK_IF_NOT_SUCCESS(create_dyn_array(mesh.vertices, storage.pool.allocator, vertex_count))
	{
		BLK_ERROR("Failed to create vertices dynamic array\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	// Allocate `mesh.indices` with capacity `index_count`.

	BLK_IF_NOT_SUCCESS(create_dyn_array(mesh.indices, storage.pool.allocator, index_count))
	{
		BLK_ERROR("Failed to create indices dynamic array\n");
		destroy_dyn_array(mesh.vertices);
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	for (uint32_t i = 0; i < vertex_count; i++)
	{
		// Initialize `Vertex_UV`.
		Vertex_UV vertex = {};

		BLK_IF_NOT_SUCCESS(read(serial, vertex))
		{
			BLK_ERROR("Failed to read vertex #%u\n", i);
			destroy_dyn_array(mesh.vertices);
			destroy_dyn_array(mesh.indices);
			free(*storage.pool.allocator, serial.buffer);

			return {};
		}

		push(mesh.vertices, vertex);
	}

	// Warning: we are copying the indices manually to the indices array. Because of that we bypass `Serial`, so we have
	// to check the bounds and update both `serial.position` and the `count` field inside `mesh.indices` ourselves.
	const size_t indices_size = sizeof(Index) * index_count;

	if (serial.position + indices_size > serial.size)
	{
		BLK_ERROR("Index data does not fit in mesh `%llu`\n", hash);
		destroy_dyn_array(mesh.vertices);
		destroy_dyn_array(mesh.indices);
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	memcpy(mesh.indices.buffer, serial.buffer + serial.position, indices_size);
	serial.position += indices_size;
	mesh.indices.count = index_count;

	// We have to free `serial.buffer` because it is allocated by `load_resource`.
	free(*storage.pool.allocator, serial.buffer);

	return store_resource(storage, mesh, hash, stem);
}

blk::Pool_Handle<blk::Mesh>
blk::compute_uv_sphere(const float radius, const uint32_t segment_count, const uint32_t ring_count)
{
	// Since this is a procedurally generated mesh, we need to compute a logical stem to use it to then compute its
	// mesh hash.
	char stem[MAX_RESOURCE_STEM_SIZE];

	if (const int written = snprintf(stem, sizeof(stem), "UV_Sphere:%.4f:%u:%u", radius, segment_count, ring_count);
		!BLK_VERIFY(written > 0 && static_cast<size_t>(written) < MAX_RESOURCE_STEM_SIZE))
	{
		return {};
	}

	// Compute the mesh hash.
	const uint64_t hash = get_resource_hash(stem, Resource_Type::MESH);

	if (is_resource_loaded(storage, hash))
	{
		// If the mesh is already loaded, we return its handle.
		return get_resource_handle(storage, hash);
	}

	// Initialize a `Mesh`.
	Mesh mesh = {};

	// Both counts are known up front, so we allocate them exactly instead of letting `push` grow the arrays. The rings
	// and the segments are closed with a duplicated vertex, hence the `+ 1`, and every quad pushes two triangles.
	const size_t vertex_count = (static_cast<size_t>(ring_count) + 1) * (static_cast<size_t>(segment_count) + 1);
	const size_t index_count = static_cast<size_t>(ring_count) * segment_count * 6;

	BLK_IF_NOT_SUCCESS(create_dyn_array(mesh.vertices, storage.pool.allocator, vertex_count))
	{
		BLK_ERROR("Failed to allocate vertices array\n");

		return {};
	}

	BLK_IF_NOT_SUCCESS(create_dyn_array(mesh.indices, storage.pool.allocator, index_count))
	{
		BLK_ERROR("Failed to allocate indices array\n");
		destroy_dyn_array(mesh.vertices);

		return {};
	}

	// TODO (Knowledge): This code is a copy-paste from the internet but I do not really understand it yet.

	// Compute vertices.

	for (uint32_t i = 0; i <= ring_count; i++)
	{
		const float phi = static_cast<float>(PI) * static_cast<float>(i) / static_cast<float>(ring_count);
		const float ring_height = radius * cosf(phi);
		const float ring_radius = radius * sinf(phi);

		for (uint32_t j = 0; j <= segment_count; j++)
		{
			const float theta =
				2.0f * static_cast<float>(PI) * static_cast<float>(j) / static_cast<float>(segment_count);

			Vertex_UV vertex = {};
			vertex.position.x = ring_radius * cosf(theta);
			vertex.position.y = ring_height;
			vertex.position.z = ring_radius * sinf(theta);

			vertex.normal = compute_unit_vector(vertex.position);

			vertex.texture_coord.x = static_cast<float>(j) / static_cast<float>(segment_count);
			vertex.texture_coord.y = static_cast<float>(i) / static_cast<float>(ring_count);

			// TODO (Bug): Tangent and bitangent are not being computed.

			push(mesh.vertices, vertex);
		}
	}

	// Compute indices.

	for (uint32_t i = 0; i < ring_count; i++)
	{
		uint32_t current_ring = i * (segment_count + 1);
		uint32_t next_ring = current_ring + segment_count + 1;

		for (uint32_t j = 0; j < segment_count; j++, current_ring++, next_ring++)
		{
			if (i != 0)
			{
				push(mesh.indices, current_ring);
				push(mesh.indices, next_ring);
				push(mesh.indices, current_ring + 1);
			}
			if (i != ring_count - 1)
			{
				push(mesh.indices, current_ring + 1);
				push(mesh.indices, next_ring);
				push(mesh.indices, next_ring + 1);
			}
		}
	}

	// Store mesh in `storage`.
	return store_resource(storage, mesh, hash, stem);
}

void
blk::unload_mesh(const Pool_Handle<Mesh> handle)
{
	// First, we have to destroy `Mesh::vertices` and `Mesh::indices` dynamic arrays created when a `Mesh` is
	// initialized.
	Mesh* mesh = get_mesh(handle);

	if (!mesh)
	{
		// `mesh` does not exists, so we have nothing to unload.
		return;
	}

	destroy_dyn_array(mesh->vertices);
	destroy_dyn_array(mesh->indices);

	// Release `mesh` from `storage`.
	release_resource(storage, handle);
}

blk::Mesh*
blk::get_mesh(const Pool_Handle<Mesh> handle)
{
	return get_resource(storage, handle);
}
