// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Mesh.hpp"

#include "Engine/Core/Math/Constants.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Resource.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include <format>
#include <string>
#include <vector>

namespace
{
blk::Resource_Storage<blk::Mesh> mesh_storage = {};
}  // namespace

blk::Pool_Handle<blk::Mesh>
blk::load_mesh(const char* stem)
{
	const uint64_t hash = get_resource_hash(stem, Resource_Type::MESH);
	const std::string path = std::format("Assets/{}.bmesh", hash);

	if (is_resource_loaded(mesh_storage, hash))
	{
		return get_resource_handle(mesh_storage, hash);
	}

	File* file = open_file(path.c_str(), File_Access_Mode::READ);

	if (!file)
	{
		BLK_FATAL("Failed to open asset: %s\n", path.c_str());
	}

	const uint64_t file_size = get_file_size(file);
	auto buffer = static_cast<char*>(malloc(file_size));
	read_file(file, buffer, file_size);
	close_file(file);

	Serial src_serial(buffer, file_size);

	if (src_serial.read<Magic>() != BLINK_MAGIC)
	{
		BLK_FATAL("Source buffer is not Blink format\n");
	}

	if (src_serial.read<Magic>() != MESH_MAGIC)
	{
		BLK_FATAL("Source buffer is not a Blink mesh\n");
	}

	if (src_serial.read<uint8_t>() != MESH_VERSION)
	{
		BLK_FATAL("Expected Blink mesh version %u\n", MESH_VERSION);
	}

	Mesh mesh = {};

	const auto vertex_count = src_serial.read<uint32_t>();
	mesh.vertices.reserve(vertex_count);

	const auto index_count = src_serial.read<uint32_t>();

	for (uint32_t i = 0; i < vertex_count; i++)
	{
		Vertex_PNT vertex = {};

		vertex.position.x = src_serial.read<float>();
		vertex.position.y = src_serial.read<float>();
		vertex.position.z = src_serial.read<float>();

		vertex.normal.x = src_serial.read<float>();
		vertex.normal.y = src_serial.read<float>();
		vertex.normal.z = src_serial.read<float>();

		vertex.texture_coord.x = src_serial.read<float>();
		vertex.texture_coord.y = src_serial.read<float>();

		mesh.vertices.push_back(vertex);
	}

	mesh.indices.resize(index_count);
	memcpy(mesh.indices.data(), src_serial.buffer() + src_serial.position(), sizeof(Index) * index_count);

	return store_resource(mesh_storage, mesh, hash, stem);
}

/// `segment_count`: number of vertical slices.
/// `ring_count`: number of horizontal slices.
blk::Pool_Handle<blk::Mesh>
blk::compute_uv_sphere(const float radius, const uint32_t segment_count, const uint32_t ring_count)
{
	const std::string stem = std::format("UV_Sphere:{}:{}:{}", radius, segment_count, ring_count);
	const uint64_t hash = get_resource_hash(stem.c_str(), Resource_Type::MESH);

	if (is_resource_loaded(mesh_storage, hash))
	{
		return get_resource_handle(mesh_storage, hash);
	}

	Mesh mesh = {};

	for (uint32_t i = 0; i <= ring_count; i++)
	{
		const float phi = static_cast<float>(PI) * static_cast<float>(i) / ring_count;
		const float ring_height = radius * cosf(phi);
		const float ring_radius = radius * sinf(phi);

		for (uint32_t j = 0; j <= segment_count; j++)
		{
			const float theta = 2.0f * static_cast<float>(PI) * static_cast<float>(j) / segment_count;

			Vertex_PNT vertex = {};
			vertex.position.x = ring_radius * cosf(theta);
			vertex.position.y = ring_height;
			vertex.position.z = ring_radius * sinf(theta);

			vertex.normal = compute_unit_vector(vertex.position);

			vertex.texture_coord.x = static_cast<float>(j) / segment_count;
			vertex.texture_coord.y = static_cast<float>(i) / ring_count;

			mesh.vertices.push_back(vertex);
		}
	}

	for (uint32_t i = 0; i < ring_count; i++)
	{
		uint32_t current_ring = i * (segment_count + 1);
		uint32_t next_ring = current_ring + segment_count + 1;

		for (uint32_t j = 0; j < segment_count; j++, current_ring++, next_ring++)
		{
			if (i != 0)
			{
				mesh.indices.push_back(static_cast<Index>(current_ring));
				mesh.indices.push_back(static_cast<Index>(next_ring));
				mesh.indices.push_back(static_cast<Index>(current_ring + 1));
			}
			if (i != ring_count - 1)
			{
				mesh.indices.push_back(static_cast<Index>(current_ring + 1));
				mesh.indices.push_back(static_cast<Index>(next_ring));
				mesh.indices.push_back(static_cast<Index>(next_ring + 1));
			}
		}
	}

	return store_resource(mesh_storage, mesh, hash, stem.c_str());
}

void
blk::unload_mesh(const Pool_Handle<Mesh> handle)
{
	release_resource(mesh_storage, handle);
}

blk::Mesh*
blk::get_mesh(const Pool_Handle<Mesh> handle)
{
	return get_resource(mesh_storage, handle);
}
