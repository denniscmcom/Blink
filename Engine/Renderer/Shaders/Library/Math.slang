// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

static const double PI = 3.14159265358979323846;

/// Computes the ray-sphere intersection given the sphere is at the world origin.
/// It returns `true` if they intersect and writes the entry `t0` and exit `t1` points.
bool intersect_ray_sphere(float3 origin, float3 direction, float radius, out float t0, out float t1);

bool
intersect_ray_sphere(float3 origin, float3 direction, float radius, out float t0, out float t1)
{
	// To solve the quadratic equation, we first compute the discriminant.

	float a = dot(direction, direction);
	float b = 2.0f * dot(origin, direction);
	float c = dot(origin, origin) - radius * radius;

	float D = b * b - 4.0f * a * c;

	// Compute possible solutions.

	t0 = 0.0f;
	t1 = t0;

	if (D < 0.0f)
	{
		// No real solution.
		return false;
	}

	t0 = (-b - sqrt(D)) / (2.0f * a);
	t1 = (-b + sqrt(D)) / (2.0f * a);

	return true;
}
