// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Library/Atmosphere.slang"
#include "Library/Math.slang"

/// Samples taken between two consecutive depth slices. Marching a column of 32 slices this way reaches the far end in
/// 31 samples, in line with the 30 Table 2 budgets for this LUT.
static const int STEP_COUNT_PER_SLICE = 1;

/// Computes the aerial perspective LUT of the Sébastien Hillaire's model.
/// Each invocation owns one froxel column and marches it slice by slice, so consecutive slices reuse the transmittance
/// and luminance accumulated so far instead of restarting the ray march. Section 5.4.
[shader("compute")]
[numthreads(8,8,1)]
void
cs_main(uint3 thread_id : SV_DispatchThreadID)
{
	// Get LUT dimensions.
	uint lut_width = 0;
	uint lut_height = 0;
	uint lut_depth = 0;
	skybox_aerial_storage_image.GetDimensions(lut_width, lut_height, lut_depth);
	float2 lut_size = float2(lut_width, lut_height);

	// Convert `thread_id` to UV in [0, 1]. These are screen texture coordinates, so the column runs through the center
	// of its screen tile.
	float2 uv = (float2(thread_id.xy) + 0.5f) / lut_size;

	// Normalized device coordinates are Y up and the negative viewport height flips them, so the top of the screen,
	// `uv.y = 0`, is `ndc.y = 1`.
	float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);

	// The inverse view projection has no translation, so as in `VS_Skybox.slang` it turns a point on the far plane into
	// the world space direction of the camera ray through it.
	float3 direction = normalize(mul(skybox_ubo.inversed_view_projection, float4(ndc, 1.0f, 1.0f)).xyz);
	float3 sun_direction = normalize(skybox_ubo.sun_direction);

	// Map UV to physical parameters.

	float height = max(skybox_ubo.view_height / 1000.0f, MINIMUM_VIEW_HEIGHT);
	float3 origin = float3(0.0f, skybox_ubo.planet_radius + height, 0.0f);

	// Compute the distance to integrate: find where the ray exits the atmosphere.

	float t0 = 0.0f;
	float t1 = 0.0f;

	// The ray always starts inside the atmosphere for now, so `t0` is behind us and the exit point `t1` is the far end
	// of the segment we have to integrate over.

	if (!intersect_ray_sphere(origin, direction, skybox_ubo.atmosphere_radius, t0, t1))
	{
		// There is no atmosphere, so nothing scatters and nothing is absorbed.
		for (uint slice = 0; slice < lut_depth; ++slice)
		{
			skybox_aerial_storage_image[uint3(thread_id.xy, slice)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
		}

		return;
	}

	// The far end of the ray march. The distance at which we stop accumulating.
	float t_max = t1;

	if (intersect_ray_sphere(origin, direction, skybox_ubo.planet_radius, t0, t1) && t0 > 0.0f)
	{
		// There are two things that can stop a ray:
		// - It runs out of air (no atmosphere): atmosphere's far root `t1`.
		// - It hits the ground: planet's near root `t0`.

		// This case is the second.
		t_max = t0;
	}

	// Compute phase functions `p(v, l_i)`. They only depend on the angle between the sun and the view direction, which
	// is the same along the whole column, so we compute them before the main loop.
	float cos_theta = dot(direction, sun_direction);

	float rayleigh_phase_value = rayleigh_phase(cos_theta);
	float mie_phase_value = mie_phase(cos_theta, skybox_ubo.mie_asymmetry);

	// Get transmittance LUT dimensions, needed to map each sample's parameters onto its texel centers.
	uint transmittance_lut_width = 0;
	uint transmittance_lut_height = 0;
	skybox_transmittance_lut.GetDimensions(transmittance_lut_width, transmittance_lut_height);
	float2 transmittance_lut_size = float2(transmittance_lut_width, transmittance_lut_height);

	// Get multiscattering LUT dimensions.
	uint multiscattering_lut_width = 0;
	uint multiscattering_lut_height = 0;
	skybox_multiscattering_lut.GetDimensions(multiscattering_lut_width, multiscattering_lut_height);
	float2 multiscattering_lut_size = float2(multiscattering_lut_width, multiscattering_lut_height);

	// Start at full, nothing absorbed yet. Eq. 2 `T(view -> sample_point)`.
	float3 transmittance = float3(1.0f);
	float3 luminance = float3(0.0f);

	// Where the previous slice left off. Slice 0 sits on the camera, so it integrates an empty segment.
	float previous_distance = 0.0f;

	for (uint slice = 0; slice < lut_depth; ++slice)
	{
		// Past the atmosphere boundary or the ground there is nothing left to integrate, so the remaining slices keep
		// what the column had gathered by then.
		float distance = min(aerial_slice_to_distance(float(slice), float(lut_depth)), t_max);
		float dt = (distance - previous_distance) / float(STEP_COUNT_PER_SLICE);

		for (int i = 0; i < STEP_COUNT_PER_SLICE; ++i)
		{
			// Sample at the midpoint of each segment.
			float t = previous_distance + dt * (float(i) + 0.5f);
			float3 sample_position = origin + direction * t;

			// Distance from the planet center, and the altitude above its surface, of the sample point.
			float sample_radius = length(sample_position);
			float sample_height = sample_radius - skybox_ubo.planet_radius;

			// We sample the medium at the current height.
			Medium_Sample medium_sample = sample_medium(
				sample_height,
				skybox_ubo.rayleigh_scattering,
				skybox_ubo.rayleigh_scale,
				skybox_ubo.mie_scattering,
				skybox_ubo.mie_absorption,
				skybox_ubo.mie_scale,
				skybox_ubo.ozone_absorption,
				skybox_ubo.ozone_mid_point,
				skybox_ubo.ozone_low_point
			);

			float sun_t0 = 0.0f;
			float sun_t1 = 0.0f;

			// Default to zero (shadowed).
			float3 sun_transmittance = float3(0.0f);

			// Both LUT lookups below are keyed on the sun's angle to the sample point's zenith.
			float sun_sample_cos_angle = dot(sample_position / sample_radius, sun_direction);

			// Sample point is in the planet shadow, the `Vis` of Eq. 4.
			bool in_shadow =
				intersect_ray_sphere(sample_position, sun_direction, skybox_ubo.planet_radius, sun_t0, sun_t1)
				&& sun_t0 > 0.0f;

			if (!in_shadow)
			{
				// Compute sun transmittance. Eq. 2 `T(sample_point, sun)`.
				float2 transmittance_uv = transmittance_parameters_to_uv(
					sample_radius,
					sun_sample_cos_angle,
					transmittance_lut_size,
					skybox_ubo.planet_radius,
					skybox_ubo.atmosphere_radius
				);

				// A compute shader has no quads to take derivatives across, so the level is named explicitly.
				sun_transmittance = skybox_transmittance_lut.SampleLevel(transmittance_uv, 0.0f).rgb;
			}

			// Every scattering order past the first, from the multiscattering LUT. It carries no phase function, and
			// the planet shadow does not apply to it: light arrives at these points by bouncing.
			float2 multiscattering_uv = multiscattering_parameters_to_uv(
				sample_height,
				sun_sample_cos_angle,
				multiscattering_lut_size,
				skybox_ubo.planet_radius,
				skybox_ubo.atmosphere_radius
			);

			float3 multiscattering = skybox_multiscattering_lut.SampleLevel(multiscattering_uv, 0.0f).rgb;

			// How much light scatters toward the camera at this point. Eq. 11 `L_scat`.
			float3 single_scattering = (medium_sample.S_s_rayleigh * rayleigh_phase_value
				+ medium_sample.S_s_mie * mie_phase_value)
				* sun_transmittance;
			float3 multiple_scattering = (medium_sample.S_s_rayleigh + medium_sample.S_s_mie) * multiscattering;

			float3 in_scattering = (single_scattering + multiple_scattering) * transmittance * skybox_ubo.sun_intensity;

			// Update luminance.
			luminance += in_scattering * dt;

			// Update transmittance.
			transmittance *= exp(-medium_sample.S_t * dt);
		}

		previous_distance = distance;

		// In-scattering in RGB, and in A the transmittance as the mean of its wavelengths. Section 5.4.
		float mean_transmittance = (transmittance.r + transmittance.g + transmittance.b) / 3.0f;

		skybox_aerial_storage_image[uint3(thread_id.xy, slice)] = float4(luminance, mean_transmittance);
	}
}
