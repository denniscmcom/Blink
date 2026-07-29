#include "Core/Math/Matrix.hpp"

#include "Core/Math/Unit.hpp"

#include <math.h>

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

blk::Matrix4
blk::make_identity_matrix()
{
	Matrix4 matrix{};
	matrix.columns[0].x = 1.0f;
	matrix.columns[1].y = 1.0f;
	matrix.columns[2].z = 1.0f;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::make_translation_matrix(const Vector3& translation)
{
	Matrix4 matrix = make_identity_matrix();
	matrix.columns[3].x = translation.x;
	matrix.columns[3].y = translation.y;
	matrix.columns[3].z = translation.z;

	return matrix;
}

blk::Matrix4
blk::make_scale_matrix(const Vector3& scale)
{
	Matrix4 matrix{};
	matrix.columns[0].x = scale.x;
	matrix.columns[1].y = scale.y;
	matrix.columns[2].z = scale.z;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::make_rotation_matrix_x(const Radians angle)
{
	const float cos = cosf(angle.value);
	const float sin = sinf(angle.value);

	Matrix4 matrix{};
	matrix.columns[0].x = 1.0f;
	matrix.columns[1].y = cos;
	matrix.columns[1].z = -sin;
	matrix.columns[2].y = sin;
	matrix.columns[2].z = cos;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::make_rotation_matrix_y(const Radians angle)
{
	const float cos = cosf(angle.value);
	const float sin = sinf(angle.value);

	Matrix4 matrix{};
	matrix.columns[0].x = cos;
	matrix.columns[0].z = sin;
	matrix.columns[1].y = 1.0f;
	matrix.columns[2].x = -sin;
	matrix.columns[2].z = cos;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::make_rotation_matrix_z(const Radians angle)
{
	const float cos = cosf(angle.value);
	const float sin = sinf(angle.value);

	Matrix4 matrix{};
	matrix.columns[0].x = cos;
	matrix.columns[0].y = -sin;
	matrix.columns[1].x = sin;
	matrix.columns[1].y = cos;
	matrix.columns[2].z = 1.0f;
	matrix.columns[3].w = 1.0f;

	return matrix;
}

blk::Matrix4
blk::make_rotation_matrix(const Vector3& rotation)
{
	const Matrix4 rotation_x = make_rotation_matrix_x(Radians{rotation.x});
	const Matrix4 rotation_y = make_rotation_matrix_y(Radians{rotation.y});
	const Matrix4 rotation_z = make_rotation_matrix_z(Radians{rotation.z});

	return rotation_y * rotation_x * rotation_z;
}

blk::Matrix4
blk::make_look_at_matrix(const Vector3& eye, const Vector3& target, const Vector3& up)
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
blk::make_perspective_matrix(const Radians fov, const float aspect_ratio, const float near, const float far)
{
	Matrix4 matrix = make_identity_matrix();

	const float tan_half_fov = tanf(fov.value * 0.5f);
	matrix.columns[0].x = 1.0f / (tan_half_fov * aspect_ratio);

	matrix.columns[2].w = 1.0f;
	matrix.columns[3].w = 0.0f;

	matrix.columns[2].z = far / (far - near);
	matrix.columns[3].z = -(far * near) / (far - near);

	// TODO: Not sure about this flip here.
	matrix.columns[1].y = -1.0f / tan_half_fov;

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

std::optional<blk::Matrix4>
blk::inverse(const Matrix4& matrix)
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
		return std::nullopt;
	}

	const float inverse_determinant = 1.0f / determinant;
	Matrix4 result = {};

	for (int column = 0; column < 4; column++)
	{
		result.columns[column] = inverse_determinant * Vector4{
														   .x = cofactors[column][0],
														   .y = cofactors[column][1],
														   .z = cofactors[column][2],
														   .w = cofactors[column][3]
													   };
	}

	return result;
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
