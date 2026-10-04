// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Renderer/Resource/Material_Device.hpp"
#include "Engine/Renderer/Resource/Mesh_Device.hpp"

#include <stddef.h>

namespace blk
{
struct Pipeline;

// Constants may be updated per draw call multiple times in a frame.
//
// Push constant structures must stay within 128 bytes. It is the minimum `maxPushConstantsSize` the Vulkan
// specification guarantees (see the required limits table in `External/Vulkan-Docs-main/chapters/limits.adoc`).

/// Per-mesh parameters.
struct Mesh_Constants
{
	/// Mesh model matrix.
	Matrix4 model;
	/// Mesh normal matrix.
	Matrix4 normal_matrix;
};

/// Per-terrain parameters.
/// It should match the GPU-side `Terrain_Constants` structure in `Shaders/Interface/Terrain_Constants.slang`.
///
/// It has no normal matrix. Terrain nodes are only translated, so the normals `VS_Terrain.slang` computes need no
/// transform.
struct Terrain_Constants
{
	/// Terrain model matrix.
	Matrix4 model;
	/// `Terrain_Ref::uv_offset`.
	Vector2 uv_offset;
	/// `Terrain_Ref::uv_scale`.
	float uv_scale;
};

static_assert(sizeof(Terrain_Constants) == 64 + 8 + 4);
static_assert(offsetof(Terrain_Constants, model) == 0);
static_assert(offsetof(Terrain_Constants, uv_offset) == 64);
static_assert(offsetof(Terrain_Constants, uv_scale) == 64 + 8);

/// Data needed by the renderer to issue a single draw command.
///
/// Many draw commands are issued per frame.
struct Draw_Command
{
	/// Whether this is a mesh, a terrain or the skybox is decided by which array it lives in —
	/// `Frame::mesh_draw_commands`, `Frame::terrain_draw_commands` or `Frame::skybox_draw_commands` — so only the
	/// fields relevant to that array are populated.

	/// Pipeline to use when issuing the draw command.
	Pipeline* pipeline;
	/// Mesh parameters.
	Mesh_Constants mesh_constants;
	/// Terrain parameters.
	Terrain_Constants terrain_constants;
	///	Mesh data uploaded to device.
	Mesh_Device mesh_device;
	/// Material data uploaded to device.
	Material_Device material_device;
};
}  // namespace blk
