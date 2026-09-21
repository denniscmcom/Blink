// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Library/Math.slang"

// The three terms of the Cook-Torrance microfacet specular BRDF:
//
//            D(h) * G(l, v) * F(h, v)
//   f_spec = ------------------------
//              4 * (n . v) * (n . l)
//
// `D` is the normal distribution function, `G` the geometry function, and `F` the Fresnel term.

/// Trowbridge-Reitz GGX normal distribution function.
/// It gives the fraction of microfacets aligned with the halfway direction.
float distribution_ggx(float normal_dot_halfway_direction, float roughness);
/// Schlick-GGX approximation of the geometry function for a single direction.
float geometry_schlick_ggx(float normal_dot_v, float roughness);
/// Smith's geometry function. It accounts for microfacets shadowing each other, both along the view direction and
/// along the light direction.
float geometry_smith(float normal_dot_view_direction, float normal_dot_light_direction, float roughness);
/// Schlick approximation of the Fresnel term.
/// @param cos_theta Cosine of the angle between the halfway direction and the view direction.
/// @param F0 Reflectance at normal incidence.
float3 fresnel_schlick(float cos_theta, float3 F0);

float
distribution_ggx(float normal_dot_halfway_direction, float roughness)
{
	float roughness4 = pow(roughness, 4);
	float normal_dot_halfway_direction2 = pow(normal_dot_halfway_direction, 2);

	return roughness4 / (pow((normal_dot_halfway_direction2 * (roughness4 - 1.0) + 1.0), 2) * (float)PI);
}

float
geometry_schlick_ggx(float normal_dot_v, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	float num = normal_dot_v;
	float denom = normal_dot_v * (1.0 - k) + k;

	return num / denom;
}

float
geometry_smith(float normal_dot_view_direction, float normal_dot_light_direction, float roughness)
{
	float ggx2 = geometry_schlick_ggx(normal_dot_view_direction, roughness);
	float ggx1 = geometry_schlick_ggx(normal_dot_light_direction, roughness);

	return ggx1 * ggx2;
}

float3
fresnel_schlick(float cos_theta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}
