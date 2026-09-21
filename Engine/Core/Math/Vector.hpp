// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

/// Host-side vectors up to 4 dimensions.

#pragma once

namespace blk
{
struct Matrix4;
enum class Result;

struct Vector2
{
	float x;
	float y;

	/// @warning Crashes at runtime if `index` is out of bounds.
	float& operator[](int index);
	/// @warning Crashes at runtime if `index` is out of bounds.
	float operator[](int index) const;
};

struct Vector3
{
	float x;
	float y;
	float z;

	/// @warning Crashes at runtime if `index` is out of bounds.
	float& operator[](int index);
	/// @warning Crashes at runtime if `index` is out of bounds.
	float operator[](int index) const;
};

struct Vector4
{
	float x;
	float y;
	float z;
	float w;

	/// @warning Crashes at runtime if `index` is out of bounds.
	float& operator[](int index);
	/// @warning Crashes at runtime if `index` is out of bounds.
	float operator[](int index) const;
};

Vector2 operator+(const Vector2& lhs, const Vector2& rhs);
Vector2 operator-(const Vector2& lhs, const Vector2& rhs);
Vector2 operator*(float scalar, const Vector2& vector);
Vector2 operator-(const Vector2& vector);

Vector3 operator+(const Vector3& lhs, const Vector3& rhs);
Vector3 operator-(const Vector3& lhs, const Vector3& rhs);
Vector3 operator*(float scalar, const Vector3& vector);
Vector3 operator-(const Vector3& vector);
Vector3& operator+=(Vector3& lhs, const Vector3& rhs);

Vector4 operator+(const Vector4& lhs, const Vector4& rhs);
Vector4 operator-(const Vector4& lhs, const Vector4& rhs);
Vector4 operator*(float scalar, const Vector4& vector);
Vector4 operator-(const Vector4& vector);
Vector4 operator*(const Matrix4& matrix, const Vector4& vector);

Vector3 compute_cross_product(const Vector3& lhs, const Vector3& rhs);
Vector3 compute_unit_vector(const Vector3& vector);

float compute_dot_product(const Vector3& lhs, const Vector3& rhs);
/// Computes the vector magnitude squared.
float compute_vector_magnitude_squared(const Vector3& vector);
float compute_vector_magnitude(const Vector3& vector);

/// Accesses `vector` element at `index` and writes it to `value`.
Result at(const Vector2& vector, int index, float& value);
/// Accesses `vector` element at `index` and writes it to `value`.
Result at(const Vector3& vector, int index, float& value);
/// Accesses `vector` element at `index` and writes it to `value`.
Result at(const Vector4& vector, int index, float& value);
}  // namespace blk
