// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Radians
{
	float value;
};

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

Radians to_radians(Degrees degrees);
Degrees to_degrees(Radians radians);
}  // namespace blk
