// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"

namespace blk
{
template <typename Type>
struct Pool_Handle;
struct Allocator;
struct Mesh;
struct Material;
enum class Result;

/// One-to-one relationship between meshes and materials.
///
/// `mesh_handles.count` is always equal to `material_handles.count`.
///
/// @see `create_mesh_instance` to allocate both arrays.
struct Mesh_Instance
{
	/// Array of mesh handles.
	Dyn_Array<Pool_Handle<Mesh>> mesh_handles;
	/// Array of material handles.
	Dyn_Array<Pool_Handle<Material>> material_handles;
};

/// Creates the mesh and material arrays of `mesh_instance`, both with `capacity` entries.
Result create_mesh_instance(Mesh_Instance& mesh_instance, Allocator* allocator, size_t capacity);
/// Destroys the mesh and material arrays of `mesh_instance`.
void destroy_mesh_instance(Mesh_Instance& mesh_instance);
}  // namespace blk
