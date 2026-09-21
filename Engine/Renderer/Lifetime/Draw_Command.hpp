// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Renderer/Resource/Material_Device.hpp"
#include "Engine/Renderer/Resource/Mesh_Device.hpp"

#include <stddef.h>

namespace blk
{
struct Pipeline;

// Constants may be updated per draw call multiple times in a frame.

/// Per-mesh parameters.
struct Mesh_Constants
{
	/// Mesh model matrix.
	Matrix4 model;
	/// Mesh normal matrix.
	Matrix4 normal_matrix;
};

/// Data needed by the renderer to issue a single draw command.
///
/// Many draw commands are issued per frame.
struct Draw_Command
{
	/// Whether this is a mesh or the skybox is decided by which array it lives in — `Frame::mesh_draw_commands` or
	/// `Frame::skybox_draw_commands` — so only the fields relevant to that array are populated.

	/// Pipeline to use when issuing the draw command.
	Pipeline* pipeline;
	/// Mesh parameters.
	Mesh_Constants mesh_constants;
	///	Mesh data uploaded to device.
	Mesh_Device mesh_device;
	/// Material data uploaded to device.
	Material_Device material_device;
};
}  // namespace blk
