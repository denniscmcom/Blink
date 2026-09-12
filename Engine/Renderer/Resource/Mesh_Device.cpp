// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Resource/Mesh_Device.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Resource/Mesh.hpp"

blk::Result
blk::transfer_mesh(const Context& context, Arena& arena, const Pool_Handle<Mesh>& host_handle, Mesh_Device& mesh)
{
	mesh = {};

	// Check if `mesh` already exist in device.
	if (const Mesh_Device* mesh_ptr = get(arena.meshes, host_handle))
	{
		mesh = *mesh_ptr;

		return Result::SUCCESS;
	}

	// Get host mesh data.

	const Mesh* mesh_host = get_mesh(host_handle);

	if (!mesh_host)
	{
		BLK_ERROR("Failed to get mesh host data to transfer\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Verify if `mesh` fits in vertex and index buffer.

	const VkDeviceSize available_vertex_buffer_size = arena.vertex_buffer.host.size - arena.vertex_buffer_offset;
	const VkDeviceSize available_index_buffer_size = arena.index_buffer.host.size - arena.index_buffer_offset;

	if (available_vertex_buffer_size < sizeof(Vertex_UV) * mesh_host->vertices.count)
	{
		BLK_ERROR("Vertex buffer is too small\n");

		return Result::OUT_OF_MEMORY;
	}

	if (available_index_buffer_size < sizeof(Index) * mesh_host->indices.count)
	{
		BLK_ERROR("Index buffer is too small\n");

		return Result::OUT_OF_MEMORY;
	}

	mesh.host_handle = host_handle;

	if (!arena.vertex_buffer.host.map)
	{
		// Map vertex buffer if it is not already.
		map_buffer(context, arena.vertex_buffer.host);
	}

	if (!arena.index_buffer.host.map)
	{
		// Map index buffer if it is not already.
		map_buffer(context, arena.index_buffer.host);
	}

	// Update vertex buffer

	if (const Result result = update_buffer(
			arena.vertex_buffer.host,
			mesh_host->vertices.buffer,
			sizeof(Vertex_UV) * mesh_host->vertices.count,
			arena.vertex_buffer_offset
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to update vertex buffer\n");
		unload_mesh_from_device(context, arena, mesh);

		return result;
	}

	mesh.vertex_count = static_cast<uint32_t>(mesh_host->vertices.count);
	mesh.first_vertex = arena.vertex_buffer_offset / sizeof(Vertex_UV);
	arena.vertex_buffer_offset += static_cast<uint32_t>(sizeof(Vertex_UV) * mesh_host->vertices.count);

	// Update index buffer.

	if (const Result result = update_buffer(
			arena.index_buffer.host,
			mesh_host->indices.buffer,
			sizeof(Index) * mesh_host->indices.count,
			arena.index_buffer_offset
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to update index buffer\n");
		unload_mesh_from_device(context, arena, mesh);

		return result;
	}

	mesh.index_count = static_cast<uint32_t>(mesh_host->indices.count);
	mesh.first_index = arena.index_buffer_offset / sizeof(Index);
	arena.index_buffer_offset += static_cast<uint32_t>(sizeof(Index) * mesh_host->indices.count);

	// We are done updating the buffers, so we unmap them.

	unmap_buffer(context, arena.vertex_buffer.host);
	unmap_buffer(context, arena.index_buffer.host);

	// Copy data from host buffer to device buffer.

	copy_buffer(context, arena.vertex_buffer.host, arena.vertex_buffer.device);
	copy_buffer(context, arena.index_buffer.host, arena.index_buffer.device);

	// Insert host_handle-mesh pair into hash map.
	insert(arena.meshes, host_handle, mesh);

	return Result::SUCCESS;
}

void
blk::unload_mesh_from_device(const Context& context, Arena& arena, Mesh_Device& mesh)
{
	// TODO (Bug): The vertex and index ranges cannot be reclaimed. `Arena` slices its buffers with a bump offset and
	// has no free list, so the space this mesh occupies stays taken until the whole arena is destroyed.
	remove(arena.meshes, mesh.host_handle);

	mesh = {};
}
