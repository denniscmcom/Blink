// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Library/Atmosphere.slang"
#include "Library/Math.slang"

/// Step between samples along each ray (sample_point -> atmosphere boundary or ground).
static const int STEP_COUNT = 20;

/// Directions the sphere around the sample point is integrated over, laid out as a square grid of strata.
/// Section 5.5.3 integrates over 64 uniformly distributed directions.
static const int SQUARE_ROOT_DIRECTION_COUNT = 8;
static const int DIRECTION_COUNT = SQUARE_ROOT_DIRECTION_COUNT * SQUARE_ROOT_DIRECTION_COUNT;

/// Smallest extinction the analytical segment integration divides by.
static const float MINIMUM_EXTINCTION = 1e-9f;

/// Computes the multiscattering LUT of the Sébastien Hillaire's model. Section 5.5.
[shader("compute")]
[numthreads(8,8,1)]
void
cs_main(uint3 thread_id : SV_DispatchThreadID)
{
	// Get LUT dimensions.
	uint lut_width = 0;
	uint lut_height = 0;
	skybox_multiscattering_storage_image.GetDimensions(lut_width, lut_height);
	float2 lut_size = float2(lut_width, lut_height);

	// Convert `thread_id` to UV in [0, 1];
	float2 uv = (float2(thread_id.xy) + 0.5f) / lut_size;

	// Map UV to physical parameters.

	float height = 0.0f;
	float cos_sun_zenith_angle = 0.0f;
	multiscattering_uv_to_parameters(uv, lut_size, height, cos_sun_zenith_angle);

	// On the ground a downward ray meets the planet at a distance of zero, which reads as a miss and lets the march
	// run straight through it. Lift the sample point off the surface, as the sky-view LUT lifts the view.
	height = max(height, MINIMUM_VIEW_HEIGHT);

	// Only the altitude matters, so the sample point `x_s` can sit on the zenith axis with the sun swung down from it.
	float3 sample_position = float3(0.0f, PLANET_RADIUS + height, 0.0f);

	float sin_sun_zenith_angle = sqrt(saturate(1.0f - cos_sun_zenith_angle * cos_sun_zenith_angle));
	float3 sun_direction = float3(sin_sun_zenith_angle, cos_sun_zenith_angle, 0.0f);

	// Get transmittance LUT dimensions, needed to map each sample's parameters onto its texel centers.
	uint transmittance_lut_width = 0;
	uint transmittance_lut_height = 0;
	skybox_transmittance_lut.GetDimensions(transmittance_lut_width, transmittance_lut_height);
	float2 transmittance_lut_size = float2(transmittance_lut_width, transmittance_lut_height);

	// The spherical integrals of Eq. 5 and Eq. 7. Both march the same rays, so they are gathered in one pass.
	float3 second_order_luminance = float3(0.0f);
	float3 transfer_factor = float3(0.0f);

	// TODO (Performance): every direction is marched serially by the one invocation that owns the texel. The paper
	// spreads them across a workgroup instead and sums them through group shared memory.
	for (int direction_index = 0; direction_index < DIRECTION_COUNT; ++direction_index)
	{
		// Uniform over the sphere: the polar cosine is spread uniformly over [-1, 1], stratified on a square grid so
		// the directions cannot clump. The polar axis is the zenith, so the strata follow the altitude bands.
		float polar_unit =
			(float(direction_index / SQUARE_ROOT_DIRECTION_COUNT) + 0.5f) / float(SQUARE_ROOT_DIRECTION_COUNT);
		float azimuth_unit =
			(float(direction_index % SQUARE_ROOT_DIRECTION_COUNT) + 0.5f) / float(SQUARE_ROOT_DIRECTION_COUNT);

		float cos_polar = 1.0f - 2.0f * polar_unit;
		float sin_polar = sqrt(saturate(1.0f - cos_polar * cos_polar));
		float azimuth = 2.0f * float(PI) * azimuth_unit;

		// Eq. 5 takes the luminance arriving from `w`, which is the path marched outwards from the sample point.
		float3 direction = float3(sin_polar * cos(azimuth), cos_polar, sin_polar * sin(azimuth));

		// Compute the distance to integrate: find where the ray exits the atmosphere.

		float t0 = 0.0f;
		float t1 = 0.0f;

		// The sample point is always inside the atmosphere, so `t0` is behind us and the exit point `t1` is the far
		// end of the segment we have to integrate over.

		if (!intersect_ray_sphere(sample_position, direction, ATMOSPHERE_RADIUS, t0, t1))
		{
			// There is no atmosphere, so nothing to integrate.
			continue;
		}

		// The far end of the ray march. The distance at which we stop accumulating.
		float t_max = t1;

		// There are two things that can stop a ray:
		// - It runs out of air (no atmosphere): atmosphere's far root `t1`.
		// - It hits the ground: planet's near root `t0`.
		bool hits_ground = intersect_ray_sphere(sample_position, direction, PLANET_RADIUS, t0, t1) && t0 > 0.0f;

		if (hits_ground)
		{
			t_max = t0;
		}

		float dt = t_max / float(STEP_COUNT);

		// `L'` of Eq. 6 and `L_f` of Eq. 8, both integrated along this one ray.
		float3 luminance = float3(0.0f);
		float3 transfer_luminance = float3(0.0f);

		// Start at full, nothing absorbed yet. Eq. 2 `T(x_s, x)`.
		float3 transmittance = float3(1.0f);

		for (int i = 0; i < STEP_COUNT; ++i)
		{
			// Sample at the midpoint of each segment.
			float t = dt * (float(i) + 0.5f);
			float3 step_position = sample_position + direction * t;

			// Distance from the planet center, and the altitude above its surface, of the sample point.
			float step_radius = length(step_position);
			float step_height = step_radius - PLANET_RADIUS;

			// We sample the medium at the current height.
			Medium_Sample medium_sample = sample_medium(step_height);

			// From this sample point, we march a ray toward the sun. So we have to repeat the previous process but now
			// from the sample point towards the sun.

			// Compute the distance to integrate towards the sun.
			float sun_t0 = 0.0f;
			float sun_t1 = 0.0f;

			// Default to zero (shadowed).
			float3 sun_transmittance = float3(0.0f);

			// Sample point is in the planet shadow, the `Vis` of Eq. 4. Only the planet's own shadow is kept:
			// shadowing from mountains is dropped, as light scatters around them anyway. Section 5.5.1.
			bool in_shadow =
				intersect_ray_sphere(step_position, sun_direction, PLANET_RADIUS, sun_t0, sun_t1) && sun_t0 > 0.0f;

			if (!in_shadow)
			{
				// Compute sun transmittance. Eq. 2 `T(sample_point, sun)`.
				// Sample from transmittance LUT.

				float sun_step_cos_angle = dot(step_position / step_radius, sun_direction);
				float2 transmittance_uv =
					transmittance_parameters_to_uv(step_radius, sun_step_cos_angle, transmittance_lut_size);

				// A compute shader has no quads to take derivatives across, so picking the level implicitly would make
				// the shader require `SPV_KHR_compute_shader_derivatives`. The LUT has a single level, so asking for it
				// by hand samples exactly what an implicit lookup would have.
				sun_transmittance = skybox_transmittance_lut.SampleLevel(transmittance_uv, 0.0f).rgb;
			}

			float3 scattering = medium_sample.S_s_rayleigh + medium_sample.S_s_mie;
			float3 extinction = max(medium_sample.S_t, MINIMUM_EXTINCTION);
			float3 step_transmittance = exp(-extinction * dt);

			// Both integrals below take the segment analytically rather than as a rectangle, as Section 5.5.3
			// recommends. It accounts for the light the segment absorbs from itself, which is what keeps the transfer
			// factor of Eq. 7 inside [0, 1] and the series of Eq. 9 convergent.

			// How much light scatters toward the sample point at this step. Eq. 6, with an isotropic phase. `E_I`
			// stays at 1: the LUT stores a transfer function, not a luminance.
			float3 in_scattering = scattering * UNIFORM_PHASE * sun_transmittance;
			luminance += transmittance * (in_scattering - in_scattering * step_transmittance) / extinction;

			// Eq. 8 carries neither the shadowing term nor the phase function. Both are already accounted for in the
			// second order contribution this factor goes on to scatter around.
			transfer_luminance += transmittance * (scattering - scattering * step_transmittance) / extinction;

			// Update transmittance.
			transmittance *= step_transmittance;
		}

		if (hits_ground)
		{
			// The ray ended on the planet, so the ground's diffuse response closes it off. `L_o(p, v)` of Eq. 6.
			float3 ground_position = sample_position + direction * t_max;
			float ground_radius = length(ground_position);
			float3 ground_normal = ground_position / ground_radius;

			// A sun below the local horizon lights nothing, which also keeps the lookup below within the rays the
			// transmittance LUT holds.
			float ground_cos_sun_angle = saturate(dot(ground_normal, sun_direction));

			float2 transmittance_uv =
				transmittance_parameters_to_uv(ground_radius, ground_cos_sun_angle, transmittance_lut_size);
			float3 sun_transmittance = skybox_transmittance_lut.SampleLevel(transmittance_uv, 0.0f).rgb;

			// Lambertian, so the albedo spreads the reflected light over the hemisphere.
			luminance += transmittance * sun_transmittance * ground_cos_sun_angle * GROUND_ALBEDO / float(PI);
		}

		second_order_luminance += luminance;
		transfer_factor += transfer_luminance;
	}

	// Eq. 5 and Eq. 7 weight the sphere by `UNIFORM_PHASE`, and uniform directions weight each one by the solid angle
	// `4 * PI / DIRECTION_COUNT`. The two cancel, leaving the plain mean.
	second_order_luminance /= float(DIRECTION_COUNT);
	transfer_factor /= float(DIRECTION_COUNT);

	// Eq. 9. Every further scattering order is the previous one passed through the same transfer factor, so the whole
	// series collapses into one division.
	float3 infinite_scattering_factor = 1.0f / (1.0f - transfer_factor);

	// Eq. 10.
	float3 multiscattering = second_order_luminance * infinite_scattering_factor;

	skybox_multiscattering_storage_image[thread_id.xy] = float4(multiscattering, 1.0f);
}
