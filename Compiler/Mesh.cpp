// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Mesh.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include <ufbx.h>

#include <array>
#include <stdint.h>
#include <vector>

namespace
{
using Fbx_Magic = std::array<uint8_t, 23>;

constexpr Fbx_Magic FBX_MAGIC = {0x4B, 0x61, 0x79, 0x64, 0x61, 0x72, 0x61, 0x20, 0x46, 0x42, 0x58, 0x20,
								 0x42, 0x69, 0x6E, 0x61, 0x72, 0x79, 0x20, 0x20, 0x00, 0x1A, 0x00};

struct Packed_Vertex
{
	float position[3];
	float normal[3];
	float tangent[3];
	float bitangent[3];
	float texture_coord[2];
};

// `ufbx_generate_indices` compares vertices with `memcmp`, so padding bytes would break deduplication.
static_assert(sizeof(Packed_Vertex) == 14 * sizeof(float));
}  // namespace

// TODO: This is currently very fragile. I had very weird issues. I need to spend some time understanding the FBX format
//		or using other one.
void
blk::compile_mesh(Serial& input_serial, Serial& output_serial)
{
	if (input_serial.read<Fbx_Magic>() != FBX_MAGIC)
	{
		BLK_FATAL("Source buffer is not FBX\n");
	}

	ufbx_load_opts options = {};
	options.target_axes = ufbx_axes_left_handed_y_up;
	options.handedness_conversion_axis = UFBX_MIRROR_AXIS_X;
	options.handedness_conversion_retain_winding = true;
	options.target_unit_meters = 1.0f;
	options.normalize_normals = true;

	ufbx_error error = {};
	ufbx_scene* scene = ufbx_load_memory(input_serial.buffer(), input_serial.size(), &options, &error);

	if (!scene)
	{
		char description[512] = {};
		ufbx_format_error(description, sizeof(description), &error);
		BLK_FATAL("Failed to load FBX file: %s\n", description);
	}

	if (scene->meshes.count != 1)
	{
		BLK_FATAL("Expected exactly one mesh\n");
	}

	const ufbx_mesh* mesh = scene->meshes[0];

	if (!mesh->vertex_normal.exists || !mesh->vertex_tangent.exists || !mesh->vertex_bitangent.exists ||
		!mesh->vertex_uv.exists)
	{
		BLK_FATAL("Expected mesh with normals, tangents, bitangents, and uv\n");
	}

	if (mesh->num_faces != mesh->num_triangles)
	{
		BLK_FATAL("Expected triangulated mesh\n");
	}

	if (mesh->instances.count != 1)
	{
		BLK_FATAL("Expected exactly one mesh instance\n");
	}

	const ufbx_matrix geometry_to_world = mesh->instances.data[0]->geometry_to_world;
	const ufbx_matrix normal_to_world = ufbx_matrix_for_normals(&geometry_to_world);

	std::vector<Packed_Vertex> vertices = {};
	vertices.reserve(mesh->num_indices);

	for (size_t corner_index = 0; corner_index < mesh->num_indices; corner_index++)
	{
		const ufbx_vec3 local_position = ufbx_get_vertex_vec3(&mesh->vertex_position, corner_index);
		const ufbx_vec3 local_normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, corner_index);
		const ufbx_vec3 local_tangent = ufbx_get_vertex_vec3(&mesh->vertex_tangent, corner_index);
		const ufbx_vec3 local_bitangent = ufbx_get_vertex_vec3(&mesh->vertex_bitangent, corner_index);
		const ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, corner_index);

		const ufbx_vec3 position = ufbx_transform_position(&geometry_to_world, local_position);
		const ufbx_vec3 tangent = ufbx_vec3_normalize(ufbx_transform_direction(&normal_to_world, local_tangent));
		const ufbx_vec3 bitangent = ufbx_vec3_normalize(ufbx_transform_direction(&normal_to_world, local_bitangent));
		const ufbx_vec3 normal = ufbx_vec3_normalize(ufbx_transform_direction(&normal_to_world, local_normal));

		Packed_Vertex vertex = {};

		vertex.position[0] = static_cast<float>(position.x);
		vertex.position[1] = static_cast<float>(position.y);
		vertex.position[2] = static_cast<float>(position.z);

		vertex.normal[0] = static_cast<float>(normal.x);
		vertex.normal[1] = static_cast<float>(normal.y);
		vertex.normal[2] = static_cast<float>(normal.z);

		vertex.tangent[0] = static_cast<float>(tangent.x);
		vertex.tangent[1] = static_cast<float>(tangent.y);
		vertex.tangent[2] = static_cast<float>(tangent.z);

		vertex.bitangent[0] = static_cast<float>(bitangent.x);
		vertex.bitangent[1] = static_cast<float>(bitangent.y);
		vertex.bitangent[2] = static_cast<float>(bitangent.z);

		vertex.texture_coord[0] = static_cast<float>(uv.x);
		vertex.texture_coord[1] = 1.0f - static_cast<float>(uv.y);

		vertices.push_back(vertex);
	}

	std::vector<Index> indices(mesh->num_indices);

	ufbx_free_scene(scene);

	ufbx_vertex_stream vertex_stream = {};
	vertex_stream.data = vertices.data();
	vertex_stream.vertex_count = vertices.size();
	vertex_stream.vertex_size = sizeof(Packed_Vertex);

	ufbx_error dedup_error = {};
	const size_t unique_vertex_count =
		ufbx_generate_indices(&vertex_stream, 1, indices.data(), indices.size(), nullptr, &dedup_error);

	if (dedup_error.type != UFBX_ERROR_NONE)
	{
		BLK_FATAL("Failed to generate indices\n");
	}

	vertices.resize(unique_vertex_count);

	output_serial.write(BLINK_MAGIC);
	output_serial.write(MESH_MAGIC);
	output_serial.write(MESH_VERSION);

	output_serial.write(static_cast<uint32_t>(vertices.size()));
	output_serial.write(static_cast<uint32_t>(indices.size()));
	output_serial.write(vertices.data(), vertices.size() * sizeof(Packed_Vertex));
	output_serial.write(indices.data(), indices.size() * sizeof(Index));
}
