// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Resource/Mesh_Device.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Resource/Mesh.hpp"

blk::Mesh_Device
blk::transfer_mesh(const Context& context, Arena& arena, const Pool_Handle<Mesh>& mesh_handle)
{
	const Mesh* mesh = get_mesh(mesh_handle);

	if (!mesh)
	{
		BLK_FATAL("Failed to get mesh\n");
	}

	const uint32_t available_vertex_buffer_size = arena.vertex_buffer.host.size - arena.vertex_buffer_byte_offset;
	const uint32_t available_index_buffer_size = arena.index_buffer.host.size - arena.index_buffer_byte_offset;

	if (available_vertex_buffer_size < sizeof(Vertex_PNT) * mesh->vertices.size())
	{
		BLK_FATAL("Vertex buffer is too small\n");
	}

	if (available_index_buffer_size < sizeof(Index) * mesh->indices.size())
	{
		BLK_FATAL("Index buffer is too small\n");
	}

	if (!arena.vertex_buffer.host.map)
	{
		map_buffer(context, arena.vertex_buffer.host);
	}

	update_buffer(
		arena.vertex_buffer.host,
		mesh->vertices.data(),
		sizeof(Vertex_PNT) * mesh->vertices.size(),
		arena.vertex_buffer_byte_offset
	);

	unmap_buffer(context, arena.vertex_buffer.host);
	copy_buffer(context, arena.vertex_buffer.host, arena.vertex_buffer.device);

	if (!arena.index_buffer.host.map)
	{
		map_buffer(context, arena.index_buffer.host);
	}

	update_buffer(
		arena.index_buffer.host,
		mesh->indices.data(),
		sizeof(Index) * mesh->indices.size(),
		arena.index_buffer_byte_offset
	);

	unmap_buffer(context, arena.index_buffer.host);
	copy_buffer(context, arena.index_buffer.host, arena.index_buffer.device);

	Mesh_Device mesh_device = {};
	mesh_device.handle = mesh_handle;
	mesh_device.vertex_count = mesh->vertices.size();
	mesh_device.vertex_buffer_offset = arena.vertex_buffer_byte_offset / sizeof(Vertex_PNT);
	mesh_device.index_count = mesh->indices.size();
	mesh_device.index_buffer_offset = arena.index_buffer_byte_offset / sizeof(Index);

	arena.vertex_buffer_byte_offset += sizeof(Vertex_PNT) * mesh->vertices.size();
	arena.index_buffer_byte_offset += sizeof(Index) * mesh->indices.size();

	arena.meshes.insert({mesh_handle, mesh_device});

	return mesh_device;
}
