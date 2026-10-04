// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Material_Set.slang"
#include "Library/Atmosphere.slang"
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

		float light_distance = length(light_position - input.fragment_position);
		float light_attenuation = 1.0 / (light_distance * light_distance);
		float light_strength = 200.0;
		float3 light_radiance = light_color * light_attenuation * light_strength;

		Lo += cook_torrance(normal, view_direction, light_direction, albedo_sample, F0, roughness, metallic)
			* light_radiance
			* max(dot(normal, light_direction), 0.0);
	}

	float3 ambient = float3(0.03) * albedo_sample * occlusion;
	float3 color = ambient + Lo;

	// Apply sun light.
	// The sun sits outside the atmosphere, so the light reaching the surface is its illuminance dimmed by the
	// transmittance toward it: the `T(x, x + t_atmo * l_i)` of Eq. 4.

	float3 sun_direction = normalize(skybox_ubo.sun_direction);

	// Same flat-world placement as the skybox passes: the surface sits on the planet's up axis at its world height.
	float surface_height = max(input.fragment_position.y / 1000.0f, MINIMUM_VIEW_HEIGHT);
	float surface_radius = skybox_ubo.planet_radius + surface_height;
	float3 surface_position = float3(0.0f, surface_radius, 0.0f);

	float3 sun_illuminance = float3(0.0f);

	float t0 = 0.0f;
	float t1 = 0.0f;

	// Once the sun is below the horizon the planet hides it, the `Vis` of Eq. 4.
	bool sun_hidden =
		intersect_ray_sphere(surface_position, sun_direction, skybox_ubo.planet_radius, t0, t1) && t0 > 0.0f;

	if (!sun_hidden)
	{
		uint transmittance_lut_width = 0;
		uint transmittance_lut_height = 0;
		skybox_transmittance_lut.GetDimensions(transmittance_lut_width, transmittance_lut_height);

		// The surface is on the up axis, so the sun's zenith cosine is its own Y component.
		float2 transmittance_uv = transmittance_parameters_to_uv(
			surface_radius,
			sun_direction.y,
			float2(transmittance_lut_width, transmittance_lut_height),
			skybox_ubo.planet_radius,
			skybox_ubo.atmosphere_radius
		);

		// The level is named because only part of a quad may reach this branch, which leaves the derivatives an
		// implicit lookup would need undefined. The LUT has a single level, so it samples the same texels anyway.
		sun_illuminance = skybox_ubo.sun_intensity * skybox_transmittance_lut.SampleLevel(transmittance_uv, 0.0f).rgb;
	}

	color += cook_torrance(normal, view_direction, sun_direction, albedo_sample, F0, roughness, metallic)
		* sun_illuminance
		* max(dot(normal, sun_direction), 0.0);

	// Apply aerial perspective, Section 5.4.
	// The LUT is laid out over the screen, so find the froxel column this fragment falls in by projecting it as
	// `CS_Skybox_Aerial.slang` did: NDC is Y up, texture rows run top to bottom.

	float4 clip_position = mul(camera_ubo.projection, mul(camera_ubo.view, float4(input.fragment_position, 1.0f)));
	float2 ndc = clip_position.xy / clip_position.w;
	float2 screen_uv = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);

	uint aerial_width = 0;
	uint aerial_height = 0;
	uint aerial_depth = 0;
	skybox_aerial_lut.GetDimensions(aerial_width, aerial_height, aerial_depth);

	// The slices are spread over the distance along the camera ray in kilometers, and the world is in meters.
	float view_distance = length(input.fragment_position - camera_ubo.view_position) / 1000.0f;
	float aerial_w = aerial_distance_to_w(view_distance, float(aerial_depth));

	float4 aerial = skybox_aerial_lut.Sample(float3(screen_uv, aerial_w));

	// The air in front of the surface dims it, and adds the light it scatters toward the camera. It has to happen
	// before the tone map below, since the LUT holds linear luminance.
	color = color * aerial.a + aerial.rgb;

	// TODO (Consistency): only meshes are tone mapped, `FS_Skybox.slang` writes raw luminance. Equal luminance then
	// shows darker on a mesh than in the sky, so distant geometry cannot blend into the horizon through aerial
	// perspective. Tone map both with the same function, or render both into an HDR target and tone map once.
	color = color / (color + float3(1.0));

	return float4(color, 1.0);
}
