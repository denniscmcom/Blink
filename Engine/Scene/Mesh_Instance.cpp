// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Scene/Mesh_Instance.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

blk::Result
blk::create_mesh_instance(Mesh_Instance& mesh_instance, Allocator* allocator, const size_t capacity)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	mesh_instance = {};

	BLK_SUCCESS_OR_RETURN(create_dyn_array(mesh_instance.mesh_handles, allocator, capacity));

	if (const Result result = create_dyn_array(mesh_instance.material_handles, allocator, capacity);
		result != Result::SUCCESS)
	{
		destroy_dyn_array(mesh_instance.mesh_handles);

		return result;
	}

	return Result::SUCCESS;
}

void
blk::destroy_mesh_instance(Mesh_Instance& mesh_instance)
{
	destroy_dyn_array(mesh_instance.mesh_handles);
	destroy_dyn_array(mesh_instance.material_handles);

	mesh_instance = {};
}
