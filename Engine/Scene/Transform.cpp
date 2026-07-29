#include "Scene/Transform.hpp"

#include "Core/Math/Matrix.hpp"

blk::Matrix4
blk::make_transform_matrix(const Transform& transform)
{
	const Matrix4 translation_matrix = make_translation_matrix(transform.position);
	const Matrix4 rotation_matrix = make_rotation_matrix(transform.rotation);
	const Matrix4 scale_matrix = make_scale_matrix(transform.scale);

	return translation_matrix * rotation_matrix * scale_matrix;
}
