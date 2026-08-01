// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Math/Vector.hpp"

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Platform/Log.hpp"

#include <math.h>

blk::Vector2
blk::operator+(const Vector2& lhs, const Vector2& rhs)
{
	return Vector2{.x = lhs.x + rhs.x, .y = lhs.y + rhs.y};
}

blk::Vector2
blk::operator-(const Vector2& lhs, const Vector2& rhs)
{
	return Vector2{.x = lhs.x - rhs.x, .y = lhs.y - rhs.y};
}

blk::Vector2
blk::operator*(const float scalar, const Vector2& vector)
{
	return Vector2{.x = scalar * vector.x, .y = scalar * vector.y};
}

blk::Vector2
blk::operator-(const Vector2& vector)
{
	return Vector2{.x = -vector.x, .y = -vector.y};
}

blk::Vector3
blk::operator+(const Vector3& lhs, const Vector3& rhs)
{
	return Vector3{.x = lhs.x + rhs.x, .y = lhs.y + rhs.y, .z = lhs.z + rhs.z};
}

blk::Vector3
blk::operator-(const Vector3& lhs, const Vector3& rhs)
{
	return Vector3{.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z};
}

blk::Vector3
blk::operator*(const float scalar, const Vector3& vector)
{
	return Vector3{.x = scalar * vector.x, .y = scalar * vector.y, .z = scalar * vector.z};
}

blk::Vector3
blk::operator-(const Vector3& vector)
{
	return Vector3{.x = -vector.x, .y = -vector.y, .z = -vector.z};
}

blk::Vector3&
blk::operator+=(Vector3& lhs, const Vector3& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;

	return lhs;
}

float&
blk::Vector3::operator[](const int index)
{
	switch (index)
	{
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	default:
		BLK_FATAL("Index out of bounds\n");
	}
}

float
blk::Vector3::operator[](const int index) const
{
	switch (index)
	{
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	default:
		BLK_FATAL("Index out of bounds\n");
	}
}

blk::Vector4
blk::operator+(const Vector4& lhs, const Vector4& rhs)
{
	return Vector4{.x = lhs.x + rhs.x, .y = lhs.y + rhs.y, .z = lhs.z + rhs.z, .w = lhs.w + rhs.w};
}

blk::Vector4
blk::operator-(const Vector4& lhs, const Vector4& rhs)
{
	return Vector4{.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z, .w = lhs.w - rhs.w};
}

blk::Vector4
blk::operator*(const float scalar, const Vector4& vector)
{
	return Vector4{.x = scalar * vector.x, .y = scalar * vector.y, .z = scalar * vector.z, .w = scalar * vector.w};
}

blk::Vector4
blk::operator-(const Vector4& vector)
{
	return Vector4{.x = -vector.x, .y = -vector.y, .z = -vector.z, .w = -vector.w};
}

float&
blk::Vector4::operator[](const int index)
{
	switch (index)
	{
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	case 3:
		return w;
	default:
		BLK_FATAL("Index out of bounds\n");
	}
}

float
blk::Vector4::operator[](const int index) const
{
	switch (index)
	{
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	case 3:
		return w;
	default:
		BLK_FATAL("Index out of bounds\n");
	}
}

blk::Vector4
blk::operator*(const Matrix4& matrix, const Vector4& vector)
{
	return vector.x * matrix.columns[0] + vector.y * matrix.columns[1] + vector.z * matrix.columns[2] +
		   vector.w * matrix.columns[3];
}

blk::Vector3
blk::compute_cross_product(const Vector3& lhs, const Vector3& rhs)
{
	return Vector3{
		.x = lhs.y * rhs.z - lhs.z * rhs.y,
		.y = lhs.z * rhs.x - lhs.x * rhs.z,
		.z = lhs.x * rhs.y - lhs.y * rhs.x
	};
}

blk::Vector3
blk::compute_unit_vector(const Vector3& vector)
{
	const float magnitude = compute_vector_magnitude(vector);

	return Vector3{.x = vector.x / magnitude, .y = vector.y / magnitude, .z = vector.z / magnitude};
}

float
blk::compute_dot_product(const Vector3& lhs, const Vector3& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

float
blk::compute_vector_magnitude_squared(const Vector3& vector)
{
	return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

float
blk::compute_vector_magnitude(const Vector3& vector)
{
	return sqrtf(compute_vector_magnitude_squared(vector));
}
