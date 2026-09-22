// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Global_Set.slang"
#include "Library/Atmosphere.slang"
#include "Library/Math.slang"

/// Step between samples along the ray (view -> sample_point).
static const int STEP_COUNT = 30;

/// Computes the Sky-View LUT of the Sébastien Hillaire's model. Section 5.3.
[shader("compute")]
[numthreads(5,5,1)]
void
cs_main(uint3 thread_id : SV_DispatchThreadID)
{
	// Get LUT dimensions.
	uint lut_width = 0;
	uint lut_height = 0;
	skybox_sky_view_storage_image.GetDimensions(lut_width, lut_height);
	float2 lut_size = float2(lut_width, lut_height);

	// Convert `thread_id` to UV in [0, 1];
	float2 uv = (float2(thread_id.xy) + 0.5f) / lut_size;

	// Map UV to physical parameters.

	float height = max(skybox_ubo.view_height / 1000.0f, MINIMUM_VIEW_HEIGHT);
	float3 origin = float3(0.0f, PLANET_RADIUS + height, 0.0f);

	float3 direction = sky_view_uv_to_direction(uv, lut_size);
	float3 sun_direction = sky_view_canonical_sun_direction(normalize(skybox_ubo.sun_direction));

	// Compute the distance to integrate: find where the ray exits the atmosphere.

	float t0 = 0.0f;
	float t1 = 0.0f;

	// The ray always starts inside the atmosphere for now, so `t0` is behind us and the exit point `t1` is the far end
	// of the segment we have to integrate over.

	if (!intersect_ray_sphere(origin, direction, ATMOSPHERE_RADIUS, t0, t1))
	{
		// There is no atmosphere, so nothing to integrate.
		skybox_sky_view_storage_image[thread_id.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);

		return;
	}

	// The far end of the ray march. The distance at which we stop accumulating.
	float t_max = t1;

	if (intersect_ray_sphere(origin, direction, PLANET_RADIUS, t0, t1) && t0 > 0.0f)
	{
		// There are two things that can stop a ray:
		// - It runs out of air (no atmosphere): atmosphere's far root `t1`.
		// - It hits the ground: planet's near root `t0`.

		// This case is the second.
		t_max = t0;
	}

	float dt = t_max / float(STEP_COUNT);

	// Compute phase functions `p(v, l_i)`. They only depend on the angle between the sun and the view direction, so we
	// compute them before the main loop.
	float cos_theta = dot(direction, sun_direction);

	float rayleigh_phase_value = rayleigh_phase(cos_theta);
	float mie_phase_value = mie_phase(cos_theta);

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

	for (int i = 0; i < STEP_COUNT; ++i)
	{
		// Sample at the midpoint of each segment.
		float t = dt * (float(i) + 0.5f);
		float3 sample_position = origin + direction * t;

		// Distance from the planet center, and the altitude above its surface, of the sample point.
		float sample_radius = length(sample_position);
		float sample_height = sample_radius - PLANET_RADIUS;

		// We sample the medium at the current height.
		Medium_Sample medium_sample = sample_medium(sample_height);

		// From this sample point, we march a ray toward the sun. So we have to repeat the previous process but now from
		// the sample point towards the sun.

		// Compute the distance to integrate towards the sun.
		float sun_t0 = 0.0f;
		float sun_t1 = 0.0f;

		// Default to zero (shadowed).
		float3 sun_transmittance = float3(0.0f);

		// Both LUT lookups below are keyed on the sun's angle to the sample point's zenith.
		float sun_sample_cos_angle = dot(sample_position / sample_radius, sun_direction);

		// Sample point is in the planet shadow, the `Vis` of Eq. 4.
		bool in_shadow =
			intersect_ray_sphere(sample_position, sun_direction, PLANET_RADIUS, sun_t0, sun_t1) && sun_t0 > 0.0f;

		if (!in_shadow)
		{
			// Compute sun transmittance. Eq. 2 `T(sample_point, sun)`.
			// Sample from transmittance LUT.

			float2 transmittance_uv =
				transmittance_parameters_to_uv(sample_radius, sun_sample_cos_angle, transmittance_lut_size);

			// A compute shader has no quads to take derivatives across, so picking the level implicitly would make the
			// shader require `SPV_KHR_compute_shader_derivatives`. The LUT has a single level, so asking for it by hand
			// samples exactly what an implicit lookup would have.
			sun_transmittance = skybox_transmittance_lut.SampleLevel(transmittance_uv, 0.0f).rgb;
		}

		// Every scattering order past the first, from the multiscattering LUT. It carries no phase function, and the
		// planet shadow does not apply to it: light arrives at these points by bouncing.
		float2 multiscattering_uv =
			multiscattering_parameters_to_uv(sample_height, sun_sample_cos_angle, multiscattering_lut_size);
		float3 multiscattering = skybox_multiscattering_lut.SampleLevel(multiscattering_uv, 0.0f).rgb;

		// How much light scatters toward the camera at this point. Eq. 11 `L_scat`.
		float3 single_scattering = (medium_sample.S_s_rayleigh * rayleigh_phase_value
			+ medium_sample.S_s_mie * mie_phase_value)
			* sun_transmittance;
		float3 multiple_scattering = (medium_sample.S_s_rayleigh + medium_sample.S_s_mie) * multiscattering;

		float3 in_scattering = (single_scattering + multiple_scattering) * transmittance * SUN_INTENSITY;

		// Update luminance.
		luminance += in_scattering * dt;

		// Update transmittance.
		transmittance *= exp(-medium_sample.S_t * dt);
	}

	// The sun disk is not part of the LUT. It is too small for this resolution to hold and the non-linear latitude
	// mapping would distort it, so `FS_Skybox.slang` composites it after the lookup instead. Section 5.3.
	skybox_sky_view_storage_image[thread_id.xy] = float4(luminance, 1.0f);
}
