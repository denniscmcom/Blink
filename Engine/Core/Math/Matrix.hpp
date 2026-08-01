// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Vector.hpp"

#include <optional>

namespace blk
{
struct Radians;

struct Matrix2
{
	Vector2 columns[2];
};

struct Matrix3
{
	Vector3 columns[3];
};

struct alignas(16) Matrix4
{
	Vector4 columns[4];
};

Matrix4 operator*(const Matrix4& lhs, const Matrix4& rhs);

Matrix4 make_identity_matrix();
Matrix4 make_translation_matrix(const Vector3& translation);
Matrix4 make_scale_matrix(const Vector3& scale);
Matrix4 make_rotation_matrix_x(Radians angle);
Matrix4 make_rotation_matrix_y(Radians angle);
Matrix4 make_rotation_matrix_z(Radians angle);
Matrix4 make_rotation_matrix(const Vector3& rotation);
Matrix4 make_look_at_matrix(const Vector3& eye, const Vector3& target, const Vector3& up);
Matrix4 make_perspective_matrix(Radians fov, float aspect_ratio, float near, float far);
std::optional<Matrix4> inverse(const Matrix4& matrix);
Matrix4 transpose(const Matrix4& matrix);

float compute_determinant(const Matrix2& matrix);
float compute_determinant(const Matrix3& matrix);
float compute_determinant(const Matrix4& matrix);
}  // namespace blk
