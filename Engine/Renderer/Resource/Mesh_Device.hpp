#pragma once

#include "Resource/Mesh.hpp"

#include <stdint.h>

namespace blk
{
struct Context;
struct Arena;

struct Mesh_Device
{
	Pool_Handle<Mesh> handle;
	uint32_t vertex_buffer_offset;
	uint32_t vertex_count;
	uint32_t index_buffer_offset;
	uint32_t index_count;
	bool should_delete;
};

Mesh_Device transfer_mesh(const Context& context, Arena& arena, const Pool_Handle<Mesh>& mesh_handle);
}  // namespace blk
