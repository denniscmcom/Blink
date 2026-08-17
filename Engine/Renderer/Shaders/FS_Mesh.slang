// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Shared.slang"

[[vk::binding(0, 0)]]
ConstantBuffer<Camera_UBO> camera_ubo;

[[vk::binding(0, 1)]]
ConstantBuffer<Light_UBO> light_ubo;

[[vk::binding(0, 2)]]
Sampler2D albedo_map;

[[vk::binding(1, 2)]]
Sampler2D normal_map;

[[vk::binding(2, 2)]]
Sampler2D orm_map;

float geometry_schlick_ggx(float normal_dot_v, float roughness);

[shader("fragment")]
float4
fs_main(VS_Main_Output input)
	: SV_Target
{
	float3 albedo_sample = albedo_map.Sample(input.texture_coord).rgb;
	float3 normal_sample = normal_map.Sample(input.texture_coord).rgb;
	float3 orm_sample = orm_map.Sample(input.texture_coord).rgb;

	float occlusion = orm_sample.r;
	float roughness = orm_sample.g;
	float metallic = orm_sample.b;

	let tbn = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));

	float3 normal = normalize(mul(normal_sample * 2.0 - 1.0, tbn));
	float3 view_direction = normalize(camera_ubo.view_position - input.fragment_position);

	float3 F0 = lerp(float3(0.04), albedo_sample, metallic);

	// Reflectance equation.
	float3 Lo = float3(0.0);

	for (int i = 0; i < light_ubo.light_count; ++i)
	{
		// Compute per-light radiance.
		float3 light_color = light_ubo.light_colors[i];
		float3 light_position = light_ubo.light_positions[i];

		float3 light_direction = normalize(light_position - input.fragment_position);
		float3 halfway_direction = normalize(light_direction + view_direction);

		float light_distance = length(light_position - input.fragment_position);
		float light_attenuation = 1.0 / (light_distance * light_distance);
		float light_strength = 200.0;
		float3 light_radiance = light_color * light_attenuation * light_strength;

		// Cook-Torrance BRDF.

		// Distribution GGX.
		float roughness4 = pow(roughness, 4);
		float normal_dot_halfway_direction = max(dot(normal, halfway_direction), 0.0);
		float normal_dot_halfway_direction2 = pow(normal_dot_halfway_direction, 2);
		float NDF = roughness4 / (pow((normal_dot_halfway_direction2 * (roughness4 - 1.0) + 1.0), 2) * (float)PI);

		// Geometry Smith.
		float normal_dot_view_direction = max(dot(normal, view_direction), 0.0);
		float normal_dot_light_direction = max(dot(normal, light_direction), 0.0);
		float ggx2 = geometry_schlick_ggx(normal_dot_view_direction, roughness);
		float ggx1 = geometry_schlick_ggx(normal_dot_light_direction, roughness);
		float G = ggx1 * ggx2;

		// Fresnel Schlick.
		float3 cos_theta = max(dot(halfway_direction, view_direction), 0.0);
		float3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);

		float3 reflection = F;
		float3 refraction = (float3(1.0) - reflection) * (1.0 - metallic);

		float3 specular = (NDF * G * F) / (4.0 * normal_dot_view_direction * normal_dot_light_direction + 0.0001);

		Lo += (refraction * albedo_sample / (float)PI + specular) * light_radiance * normal_dot_light_direction;
	}

	float3 ambient = float3(0.03) * albedo_sample * occlusion;
	float3 color = ambient + Lo;
	color = color / (color + float3(1.0));

	return float4(color, 1.0);
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


