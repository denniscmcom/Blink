// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

namespace blk
{
template <typename Type>
struct Pool_Handle;
struct Allocator;
struct Mesh;
struct Material;
enum class Result;

/// One-to-one relationship between a mesh and a material.
/// A multi-part model is several `MESH` nodes under a `SPATIAL` one.
struct Mesh_Ref
{
	/// Array of mesh handles.
	Pool_Handle<Mesh> mesh_handle;
	/// Array of material handles.
	Pool_Handle<Material> material_handle;
};
}  // namespace blk
