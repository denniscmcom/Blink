// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Scene/Transform.hpp"

#include "Engine/Core/Math/Matrix.hpp"

blk::Matrix4
blk::init_transform_matrix(const Transform& transform)
{
	const Matrix4 translation_matrix = init_translation_matrix(transform.position);
	const Matrix4 rotation_matrix = init_rotation_matrix(transform.rotation);
	const Matrix4 scale_matrix = init_scale_matrix(transform.scale);

	return translation_matrix * rotation_matrix * scale_matrix;
}
