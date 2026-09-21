// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

/// Host-side squared matrices up to 4x4.

#pragma once

#include "Engine/Core/Math/Vector.hpp"

namespace blk
{
struct Radians;
enum class Result;

/// Column-major 2x2 matrix.
struct Matrix2
{
	Vector2 columns[2];

	/// @warning Crashes at runtime if `index` is out of bounds.
	Vector2& operator[](int index);
	/// @warning Crashes at runtime if `index` is out of bounds.
	Vector2 operator[](int index) const;
};

/// Column-major 3x3 matrix.
struct Matrix3
{
	Vector3 columns[3];

	/// @warning Crashes at runtime if `index` is out of bounds.
	Vector3& operator[](int index);
	/// @warning Crashes at runtime if `index` is out of bounds.
	Vector3 operator[](int index) const;
};

/// Column-major 4x4 matrix.
struct Matrix4
{
	Vector4 columns[4];

	/// @warning Crashes at runtime if `index` is out of bounds.
	Vector4& operator[](int index);
	/// @warning Crashes at runtime if `index` is out of bounds.
	Vector4 operator[](int index) const;
};

Matrix4 operator*(const Matrix4& lhs, const Matrix4& rhs);

/// Initializes a `Matrix3` with the upper-left 3x3 block of `matrix4`, dropping its last row and its last column. For
/// an affine transform, the dropped column is the translation, so only the linear part survives.
Matrix3 init_matrix3(const Matrix4& matrix4);

/// Initializes a `Matrix4` with `matrix3` as its upper-left 3x3 block. The added last row and last column are taken
/// from the identity, so the result is `matrix3` as an affine transform with no translation.
Matrix4 init_matrix4(const Matrix3& matrix3);
Matrix4 init_identity_matrix();
Matrix4 init_translation_matrix(const Vector3& translation);
Matrix4 init_scale_matrix(const Vector3& scale);
/// Initializes a rotation matrix around the X axis. A positive `angle` rotates `+Y` towards `+Z`.
Matrix4 init_rotation_matrix_x(Radians angle);
/// Initializes a rotation matrix around the Y axis. A positive `angle` rotates `+Z` towards `+X`.
Matrix4 init_rotation_matrix_y(Radians angle);
/// Initializes a rotation matrix around the Z axis. A positive `angle` rotates `+X` towards `+Y`.
Matrix4 init_rotation_matrix_z(Radians angle);
/// Initializes a rotation matrix from the Euler angles in `rotation`, applied in yaw–pitch–roll order.
Matrix4 init_rotation_matrix(const Vector3& rotation);
Matrix4 init_look_at_matrix(const Vector3& eye, const Vector3& target, const Vector3& up);
Matrix4 init_perspective_matrix(Radians fov, float aspect_ratio, float near_plane, float far_plane);

/// Computes the inverse of `matrix` and writes the result to `inverse_matrix`.
Result inverse(const Matrix4& matrix, Matrix4& inverse_matrix);
Matrix4 transpose(const Matrix4& matrix);

float compute_determinant(const Matrix2& matrix);
float compute_determinant(const Matrix3& matrix);
float compute_determinant(const Matrix4& matrix);

/// Accesses `matrix` column at `index` and writes it to `vector`.
Result at(const Matrix2& matrix, int index, Vector2& vector);
/// Accesses `matrix` column at `index` and writes it to `vector`.
Result at(const Matrix3& matrix, int index, Vector3& vector);
/// Accesses `matrix` column at `index` and writes it to `vector`.
Result at(const Matrix4& matrix, int index, Vector4& vector);
}  // namespace blk
