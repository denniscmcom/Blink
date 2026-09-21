// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Mesh.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Storage.hpp"

#include <ufbx.h>

#include <stdint.h>

namespace
{
/// FBX magic number.
constexpr uint8_t FBX_MAGIC[23] = {0x4B, 0x61, 0x79, 0x64, 0x61, 0x72, 0x61, 0x20, 0x46, 0x42, 0x58, 0x20,
								   0x42, 0x69, 0x6E, 0x61, 0x72, 0x79, 0x20, 0x20, 0x00, 0x1A, 0x00};
}  // namespace

// TODO (Bug): we are not handling the resizing of `output_serial` if needed.
// How to do it without much repetition?
void
blk::compiler::compile_mesh(Allocator* allocator, Serial& input_serial, Serial& output_serial)
{
	// `output_serial.buffer` should be pre-allocated by the caller.
	if (!BLK_VERIFY(allocator) || !BLK_VERIFY(output_serial.buffer))
	{
		return;
	}

	if (!input_serial.buffer)
	{
		// We do not have a mesh in buffer to compile.
		return;
	}

	// Verify FBX Magic number.

	uint8_t fbx_magic[sizeof(FBX_MAGIC)] = {};

	BLK_IF_NOT_SUCCESS(read(input_serial, fbx_magic))
	{
		BLK_FATAL("Failed to read FBX magic number\n");
	}

	if (memcmp(fbx_magic, FBX_MAGIC, sizeof(FBX_MAGIC)) != 0)
	{
		BLK_FATAL("Invalid FBX magic number; file is corrupted\n");
	}

	// UFBX load options.

	ufbx_load_opts options = {};

	// FBX is right-handed; the engine is left-handed Y-up. `ufbx` converts by mirroring one axis, and we pick X.
	options.target_axes = ufbx_axes_left_handed_y_up;
	options.handedness_conversion_axis = UFBX_MIRROR_AXIS_X;

	// Mirroring an axis reverses face winding. With this set, `ufbx` does not reverse the indices to compensate, so
	// meshes come out wound clockwise — which is what `Renderer/Lifetime/Pipeline.cpp` expects, because the renderer
	// flips Y again with a negative viewport height. Changing this requires changing `frontFace` there too.
	options.handedness_conversion_retain_winding = true;

	// The engine works in meters.
	options.target_unit_meters = 1.0f;
	options.normalize_normals = true;

	ufbx_error error = {};

	// `ufbx` parses the FBX header itself, so it needs the whole file – including the magic number we just read past.
	// Handing it `input_serial.position` instead would give it a file with no header, which it rejects as an
	// unrecognized format.
	ufbx_scene* scene = ufbx_load_memory(input_serial.buffer, input_serial.size, &options, &error);

	if (!scene)
	{
		char description[512] = {};
		ufbx_format_error(description, sizeof(description), &error);
		BLK_FATAL("Failed to load FBX file: %s\n", description);
	}

	if (scene->meshes.count != 1)
	{
		// `.bmesh` files only contain exactly one mesh. We combine meshes with materials to form a "model" in at
		// runtime with `Mesh_Instance`.
		BLK_FATAL(
			"Expected exactly one mesh; export each mesh in your model into its own FBX file and then compile them one "
			"by one\n"
		);
	}

	// Get the first and unique mesh.
	const ufbx_mesh* mesh = scene->meshes[0];

	if (!mesh->vertex_normal.exists || !mesh->vertex_tangent.exists || !mesh->vertex_bitangent.exists ||
		!mesh->vertex_uv.exists)
	{
		// These properties should be computed at export from the DCC program.
		BLK_FATAL("Expected mesh with normals, tangents, bitangents, and uv\n");
	}

	if (mesh->num_faces != mesh->num_triangles)
	{
		// Should be triangulated at export from the DCC program.
		BLK_FATAL("Expected triangulated mesh\n");
	}

	// An FBX mesh is geometry that can be placed in the scene many times, each placement being an instance with its own
	// node transform. We bake one placement into the vertex data, so we need exactly one to bake.
	if (mesh->instances.count != 1)
	{
		BLK_FATAL("Expected exactly one mesh instance\n");
	}

	// Vertex positions in an FBX are in the mesh's own local space, not where the object sits in the scene. Any
	// translation, rotation or scale the artist applied to the object lives on its node, not in the geometry, so we
	// bake that node transform in here. Normals, tangents and bitangents need the inverse transpose instead, because a
	// non-uniform scale would otherwise leave them no longer perpendicular to the surface.
	const ufbx_matrix geometry_to_world = mesh->instances.data[0]->geometry_to_world;
	const ufbx_matrix normal_to_world = ufbx_matrix_for_normals(&geometry_to_world);

	// `ufbx_generate_indices` compares vertices with `memcmp`, so padding bytes would break deduplication.
	static_assert(sizeof(Vertex_UV) == 14 * sizeof(float));
	Dyn_Array<Vertex_UV> vertices = {};

	// One vertex per corner, before deduplication.
	BLK_IF_NOT_SUCCESS(create_dyn_array(vertices, allocator, mesh->num_indices))
	{
		BLK_FATAL("Failed to create dynamic array to store vertices\n");
	}

	// We iterate corners, not vertices. A corner is one use of a vertex by one face, so a cube's eight corners become
	// twenty-four entries here. That is deliberate: attributes like normals and UVs differ per face at a hard edge or a
	// UV seam, so the same position needs several distinct vertices. `ufbx_generate_indices` collapses the ones that
	// turn out identical afterwards.
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

		Vertex_UV vertex = {};

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

		push(vertices, vertex);
	}

	Dyn_Array<Index> indices = {};

	BLK_IF_NOT_SUCCESS(create_dyn_array(indices, allocator, mesh->num_indices))
	{
		BLK_FATAL("Failed to create dynamic array to store indices\n");
	}

	// `ufbx_generate_indices` writes one index per corner into the buffer without going through `push`, so we set the
	// count ourselves. Otherwise it is told the buffer holds no indices at all.
	indices.count = mesh->num_indices;

	// We can now free the UFBX memory.
	ufbx_free_scene(scene);

	// Deduplication. `ufbx_generate_indices` compares whole vertices with `memcmp`, keeps one copy of each distinct
	// one at the front of the stream, and writes an index per corner pointing at it. It returns how many survived.
	ufbx_vertex_stream vertex_stream = {};
	vertex_stream.data = vertices.buffer;
	vertex_stream.vertex_count = vertices.count;
	vertex_stream.vertex_size = sizeof(Vertex_UV);

	ufbx_error dedup_error = {};
	const size_t unique_vertex_count =
		ufbx_generate_indices(&vertex_stream, 1, indices.buffer, indices.count, nullptr, &dedup_error);

	if (dedup_error.type != UFBX_ERROR_NONE)
	{
		BLK_FATAL("Failed to generate indices\n");
	}

	// The follow writes must be in sync with the format `Formats/BMESH.bt`.

	// `ufbx_generate_indices` deduplicated the vertices in place, so everything past `unique_vertex_count` is now
	// unused. `resize` only ever grows, so we drop the tail by setting the count directly.
	vertices.count = unique_vertex_count;

	BLK_IF_NOT_SUCCESS(write(output_serial, BLINK_MAGIC))
	{
		BLK_FATAL("Failed to write Blink magic number\n");
	}

	BLK_IF_NOT_SUCCESS(write(output_serial, MESH_MAGIC))
	{
		BLK_FATAL("Failed to write Blink mesh magic number\n");
	}

	BLK_IF_NOT_SUCCESS(write(output_serial, MESH_VERSION))
	{
		BLK_FATAL("Failed to write mesh version\n");
	}

	BLK_IF_NOT_SUCCESS(write(output_serial, static_cast<uint32_t>(vertices.count)))
	{
		BLK_FATAL("Failed to write mesh count\n");
	}

	BLK_IF_NOT_SUCCESS(write(output_serial, static_cast<uint32_t>(indices.count)))
	{
		BLK_FATAL("Failed to write index count\n");
	}

	BLK_IF_NOT_SUCCESS(write(output_serial, vertices.buffer, vertices.count * sizeof(Vertex_UV)))
	{
		BLK_FATAL("Failed to write vertex array\n");
	}

	BLK_IF_NOT_SUCCESS(write(output_serial, indices.buffer, indices.count * sizeof(Index)))
	{
		BLK_FATAL("Failed to write index array\n");
	}
}
