// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Material_Set.slang"
#include "Library/BRDF.slang"
#include "Library/Math.slang"
#include "Pass/Mesh.slang"

[shader("fragment")]
float4
fs_main(VS_Mesh_Output input) : SV_Target
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

		float normal_dot_halfway_direction = max(dot(normal, halfway_direction), 0.0);
		float normal_dot_view_direction = max(dot(normal, view_direction), 0.0);
		float normal_dot_light_direction = max(dot(normal, light_direction), 0.0);
		float halfway_direction_dot_view_direction = max(dot(halfway_direction, view_direction), 0.0);

		float NDF = distribution_ggx(normal_dot_halfway_direction, roughness);
		float G = geometry_smith(normal_dot_view_direction, normal_dot_light_direction, roughness);
		float3 F = fresnel_schlick(halfway_direction_dot_view_direction, F0);

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
