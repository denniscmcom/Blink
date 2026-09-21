// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Math/Matrix.hpp"

#include "Engine/Core/Math/Unit.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <math.h>

blk::Vector2&
blk::Matrix2::operator[](int index)
{
	BLK_ENSURE(index >= 0 && index < 2);

	return columns[index];
}

blk::Vector2
blk::Matrix2::operator[](int index) const
{
	BLK_ENSURE(index >= 0 && index < 2);

	return columns[index];
}

blk::Vector3&
blk::Matrix3::operator[](int index)
{
	BLK_ENSURE(index >= 0 && index < 3);

	return columns[index];
}

blk::Vector3
blk::Matrix3::operator[](int index) const
{
	BLK_ENSURE(index >= 0 && index < 3);

	return columns[index];
}

blk::Vector4&
blk::Matrix4::operator[](int index)
{
	BLK_ENSURE(index >= 0 && index < 4);

	return columns[index];
}

blk::Vector4
blk::Matrix4::operator[](int index) const
{
	BLK_ENSURE(index >= 0 && index < 4);

	return columns[index];
}

blk::Matrix4
blk::operator*(const Matrix4& lhs, const Matrix4& rhs)
{
	Matrix4 matrix = {};
	matrix.columns[0] = lhs * rhs.columns[0];
	matrix.columns[1] = lhs * rhs.columns[1];
	matrix.columns[2] = lhs * rhs.columns[2];
	matrix.columns[3] = lhs * rhs.columns[3];

	return matrix;
}

blk::Matrix3
blk::init_matrix3(const Matrix4& matrix4)
{
	Matrix3 matrix = {};

	for (size_t i = 0; i < 3; ++i)
	{
		matrix.columns[i].x = matrix4.columns[i].x;
		matrix.columns[i].y = matrix4.columns[i].y;
		matrix.columns[i].z = matrix4.columns[i].z;
	}

	return matrix;
}

blk::Matrix4
blk::init_matrix4(const Matrix3& matrix3)
{
	Matrix4 matrix = {};

	for (size_t i = 0; i < 3; ++i)
	{
		matrix.columns[i].x = matrix3.columns[i].x;
		matrix.columns[i].y = matrix3.columns[i].y;
		matrix.columns[i].z = matrix3.columns[i].z;
	}

	// The homogeneous component. Without it the last row and column are zero and the matrix is singular.
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::init_identity_matrix()
{
	Matrix4 matrix{};
	matrix.columns[0].x = 1.0f;
	matrix.columns[1].y = 1.0f;
	matrix.columns[2].z = 1.0f;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::init_translation_matrix(const Vector3& translation)
{
	Matrix4 matrix = init_identity_matrix();
	matrix.columns[3].x = translation.x;
	matrix.columns[3].y = translation.y;
	matrix.columns[3].z = translation.z;

	return matrix;
}

blk::Matrix4
blk::init_scale_matrix(const Vector3& scale)
{
	Matrix4 matrix{};
	matrix.columns[0].x = scale.x;
	matrix.columns[1].y = scale.y;
	matrix.columns[2].z = scale.z;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::init_rotation_matrix_x(const Radians angle)
{
	const float cos = cosf(angle.value);
	const float sin = sinf(angle.value);

	Matrix4 matrix{};
	matrix.columns[0].x = 1.0f;
	matrix.columns[1].y = cos;
	matrix.columns[1].z = sin;
	matrix.columns[2].y = -sin;
	matrix.columns[2].z = cos;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::init_rotation_matrix_y(const Radians angle)
{
	const float cos = cosf(angle.value);
	const float sin = sinf(angle.value);

	Matrix4 matrix{};
	matrix.columns[0].x = cos;
	matrix.columns[0].z = -sin;
	matrix.columns[1].y = 1.0f;
	matrix.columns[2].x = sin;
	matrix.columns[2].z = cos;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::init_rotation_matrix_z(const Radians angle)
{
	const float cos = cosf(angle.value);
	const float sin = sinf(angle.value);

	Matrix4 matrix{};
	matrix.columns[0].x = cos;
	matrix.columns[0].y = sin;
	matrix.columns[1].x = -sin;
	matrix.columns[1].y = cos;
	matrix.columns[2].z = 1.0f;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::init_rotation_matrix(const Vector3& rotation)
{
	const Matrix4 rotation_x = init_rotation_matrix_x(Radians{rotation.x});
	const Matrix4 rotation_y = init_rotation_matrix_y(Radians{rotation.y});
	const Matrix4 rotation_z = init_rotation_matrix_z(Radians{rotation.z});

	return rotation_y * rotation_x * rotation_z;
}

blk::Matrix4
blk::init_look_at_matrix(const Vector3& eye, const Vector3& target, const Vector3& up)
{
	const Vector3 forward = compute_unit_vector(target - eye);
	const Vector3 right = compute_unit_vector(compute_cross_product(up, forward));
	const Vector3 local_up = compute_cross_product(forward, right);

	Matrix4 matrix{};

	matrix.columns[0].x = right.x;
	matrix.columns[0].y = local_up.x;
	matrix.columns[0].z = forward.x;

	matrix.columns[1].x = right.y;
	matrix.columns[1].y = local_up.y;
	matrix.columns[1].z = forward.y;

	matrix.columns[2].x = right.z;
	matrix.columns[2].y = local_up.z;
	matrix.columns[2].z = forward.z;

	matrix.columns[3].x = -compute_dot_product(right, eye);
	matrix.columns[3].y = -compute_dot_product(local_up, eye);
	matrix.columns[3].z = -compute_dot_product(forward, eye);

	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::init_perspective_matrix(const Radians fov, const float aspect_ratio, const float near_plane, const float far_plane)
{
	Matrix4 matrix = init_identity_matrix();

	const float tan_half_fov = tanf(fov.value * 0.5f);
	matrix.columns[0].x = 1.0f / (tan_half_fov * aspect_ratio);

	matrix.columns[2].w = 1.0f;
	matrix.columns[3].w = 0.0f;

	matrix.columns[2].z = far_plane / (far_plane - near_plane);
	matrix.columns[3].z = -(far_plane * near_plane) / (far_plane - near_plane);

	// This is Y-up, matching the engine. Whichever graphics API renders it is responsible for its own clip space.
	matrix.columns[1].y = 1.0f / tan_half_fov;

	return matrix;
}

float
blk::compute_determinant(const Matrix2& matrix)
{
	return matrix.columns[0].x * matrix.columns[1].y - matrix.columns[1].x * matrix.columns[0].y;
}

float
blk::compute_determinant(const Matrix3& matrix)
{
	const float a = matrix.columns[0].x;
	const float b = matrix.columns[1].x;
	const float c = matrix.columns[2].x;

	Matrix2 A = {};
	A.columns[0] = Vector2{.x = matrix.columns[1].y, .y = matrix.columns[1].z};
	A.columns[1] = Vector2{.x = matrix.columns[2].y, .y = matrix.columns[2].z};

	Matrix2 B = {};
	B.columns[0] = Vector2{.x = matrix.columns[0].y, .y = matrix.columns[0].z};
	B.columns[1] = Vector2{.x = matrix.columns[2].y, .y = matrix.columns[2].z};

	Matrix2 C = {};
	C.columns[0] = Vector2{.x = matrix.columns[0].y, .y = matrix.columns[0].z};
	C.columns[1] = Vector2{.x = matrix.columns[1].y, .y = matrix.columns[1].z};

	return a * compute_determinant(A) - b * compute_determinant(B) + c * compute_determinant(C);
}

float
blk::compute_determinant(const Matrix4& matrix)
{
	const float a = matrix.columns[0].x;
	const float b = matrix.columns[1].x;
	const float c = matrix.columns[2].x;
	const float d = matrix.columns[3].x;

	Matrix3 A = {};
	A.columns[0] = Vector3{.x = matrix.columns[1].y, .y = matrix.columns[1].z, .z = matrix.columns[1].w};
	A.columns[1] = Vector3{.x = matrix.columns[2].y, .y = matrix.columns[2].z, .z = matrix.columns[2].w};
	A.columns[2] = Vector3{.x = matrix.columns[3].y, .y = matrix.columns[3].z, .z = matrix.columns[3].w};

	Matrix3 B = {};
	B.columns[0] = Vector3{.x = matrix.columns[0].y, .y = matrix.columns[0].z, .z = matrix.columns[0].w};
	B.columns[1] = Vector3{.x = matrix.columns[2].y, .y = matrix.columns[2].z, .z = matrix.columns[2].w};
	B.columns[2] = Vector3{.x = matrix.columns[3].y, .y = matrix.columns[3].z, .z = matrix.columns[3].w};

	Matrix3 C = {};
	C.columns[0] = Vector3{.x = matrix.columns[0].y, .y = matrix.columns[0].z, .z = matrix.columns[0].w};
	C.columns[1] = Vector3{.x = matrix.columns[1].y, .y = matrix.columns[1].z, .z = matrix.columns[1].w};
	C.columns[2] = Vector3{.x = matrix.columns[3].y, .y = matrix.columns[3].z, .z = matrix.columns[3].w};

	Matrix3 D = {};
	D.columns[0] = Vector3{.x = matrix.columns[0].y, .y = matrix.columns[0].z, .z = matrix.columns[0].w};
	D.columns[1] = Vector3{.x = matrix.columns[1].y, .y = matrix.columns[1].z, .z = matrix.columns[1].w};
	D.columns[2] = Vector3{.x = matrix.columns[2].y, .y = matrix.columns[2].z, .z = matrix.columns[2].w};

	return a * compute_determinant(A) - b * compute_determinant(B) + c * compute_determinant(C) -
		   d * compute_determinant(D);
}

blk::Result
blk::at(const Matrix2& matrix, int index, Vector2& vector)
{
	if (index < 0 || index > 1)
	{
		return Result::OUT_OF_BOUNDS;
	}

	vector = matrix[index];

	return Result::SUCCESS;
}

blk::Result
blk::at(const Matrix3& matrix, int index, Vector3& vector)
{
	if (index < 0 || index > 2)
	{
		return Result::OUT_OF_BOUNDS;
	}

	vector = matrix[index];

	return Result::SUCCESS;
}

blk::Result
blk::at(const Matrix4& matrix, int index, Vector4& vector)
{
	if (index < 0 || index > 3)
	{
		return Result::OUT_OF_BOUNDS;
	}

	vector = matrix[index];

	return Result::SUCCESS;
}

blk::Matrix4
blk::transpose(const Matrix4& matrix)
{
	Matrix4 result = {};

	for (int column = 0; column < 4; column++)
	{
		for (int row = 0; row < 4; row++)
		{
			result.columns[column][row] = matrix.columns[row][column];
		}
	}

	return result;
}

blk::Result
blk::inverse(const Matrix4& matrix, Matrix4& inverse_matrix)
{
	float cofactors[4][4];

	for (int row = 0; row < 4; row++)
	{
		for (int column = 0; column < 4; column++)
		{
			Matrix3 minor = {};
			int minor_column = 0;

			for (int source_column = 0; source_column < 4; source_column++)
			{
				if (source_column == column)
				{
					continue;
				}

				int minor_row = 0;

				for (int source_row = 0; source_row < 4; source_row++)
				{
					if (source_row == row)
					{
						continue;
					}

					minor.columns[minor_column][minor_row] = matrix.columns[source_column][source_row];
					minor_row += 1;
				}

				minor_column += 1;
			}

			const float sign = (row + column) % 2 == 0 ? 1.0f : -1.0f;
			cofactors[row][column] = sign * compute_determinant(minor);
		}
	}

	const float determinant = matrix.columns[0][0] * cofactors[0][0] + matrix.columns[1][0] * cofactors[0][1] +
							  matrix.columns[2][0] * cofactors[0][2] + matrix.columns[3][0] * cofactors[0][3];

	if (constexpr float epsilon = 1e-6f; fabsf(determinant) < epsilon)
	{
		return Result::INVALID_ARGUMENTS;
	}

	const float inverse_determinant = 1.0f / determinant;

	for (int column = 0; column < 4; column++)
	{
		inverse_matrix.columns[column] = inverse_determinant * Vector4{
																   .x = cofactors[column][0],
																   .y = cofactors[column][1],
																   .z = cofactors[column][2],
																   .w = cofactors[column][3]
															   };
	}

	return Result::SUCCESS;
}
