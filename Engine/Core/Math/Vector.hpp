#pragma once

namespace blk
{
struct Matrix4;

// TODO: Ensure alignment and size compatability with renderer in all shared structures.

struct alignas(8) Vector2
{
	float x;
	float y;
};

struct alignas(16) Vector3
{
	float x;
	float y;
	float z;

	float& operator[](int index);
	float operator[](int index) const;
};

struct alignas(16) Vector4
{
	float x;
	float y;
	float z;
	float w;

	float& operator[](int index);
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
float compute_vector_magnitude_squared(const Vector3& vector);
float compute_vector_magnitude(const Vector3& vector);
}  // namespace blk
