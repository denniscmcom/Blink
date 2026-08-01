// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

namespace blk
{
struct Mesh;
struct Material;

struct Mesh_Instance
{
	Pool_Handle<Mesh> mesh_handle;
	Pool_Handle<Material> material_handle;
};
}  // namespace blk
