// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

/// Mathematical unit wrappers.

#pragma once

namespace blk
{
struct Vector3;

/// Angle expressed in radians.
///
/// It wraps a bare `float` so that an angle cannot be passed where the other unit is expected.
struct Radians
{
	float value;
};

/// Angle expressed in degrees.
///
/// It wraps a bare `float` so that an angle cannot be passed where the other unit is expected.
struct Degrees
{
	float value;
};

Radians operator+(Radians radians, float value);
Radians operator+(float value, Radians radians);
Radians operator-(Radians radians, float value);
Radians operator-(float value, Radians radians);
Radians operator*(float scalar, Radians radians);
Radians operator-(Radians radians);

Degrees operator+(Degrees degrees, float value);
Degrees operator+(float value, Degrees degrees);
Degrees operator-(Degrees degrees, float value);
Degrees operator-(float value, Degrees degrees);
Degrees operator*(float scalar, Degrees degrees);
Degrees operator-(Degrees degrees);

/// Converts `degrees` to radians.
Radians to_radians(Degrees degrees);
/// Converts `radians` to degrees.
Degrees to_degrees(Radians radians);
/// Converts Euler angles to a unit direction vector (spherical-to-Cartesian).
Vector3 to_cartesian(const Vector3& rotation);
}  // namespace blk
