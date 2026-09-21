// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Global_Set.slang"
#include "Library/Atmosphere.slang"
#include "Library/Math.slang"
#include "Pass/Skybox.slang"

/// Step between samplees along the ray (view -> sample_point).
static const int STEP_COUNT = 40;
static const float3 SUN_INTENSITY = float3(10.0f);

/// Renders a dynamic skybox using Sébastien Hillaire's model – A Scalable and Production Ready Sky and Atmosphere
/// Rendering Technique.
/// https://sebh.github.io/publications/egsr2020.pdf
[shader("fragment")] float4
fs_main(VS_Skybox_Output input) : SV_Target
{
	// Compute view direction.
	float3 direction = normalize(input.direction);

	// Compute sun direction.
	float3 sun_direction = normalize(skybox_ubo.sun_direction);

	// Convert view height to kilometers.
	// When view height is close to 0 (on the planet surface), artifacts may happen.
	float view_height = max(skybox_ubo.view_height / 1000.0f, MINIMUM_VIEW_HEIGHT);

	// Compute view position in planet space.
	float3 view_height_planet = float3(0.0f, PLANET_RADIUS + view_height, 0.0f);

	// Compute the distance to integrate.

	float t0 = 0.0f;
	float t1 = 0.0f;

	// The ray always starts inside the atmosphere for now, so `t0` is behind us and the exit point `t1 ` is the far end
	// of the segment we have to integrate over.

	if (!intersect_ray_sphere(view_height_planet, direction, ATMOSPHERE_RADIUS, t0, t1))
	{
		// There is no atmosphere, so nothing to integrate.
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// The far end of the ray march. The distance at which we stop accumulating.
	float t_max = t1;

	if (intersect_ray_sphere(view_height_planet, direction, PLANET_RADIUS, t0, t1) && t0 > 0.0f)
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

	// Start at full, nothing absorbed yet. Eq. 2 `T(view -> sample_point)`.
	float3 transmittance = float3(1.0f);
	float3 luminance = float3(0.0f);

	for (int i = 0; i < STEP_COUNT; ++i)
	{
		// Sample at the midpoint of each segment.
		float t = dt * (float(i) + 0.5f);
		float3 sample_position = view_height_planet + direction * t;

		// Altitude above the planet surface of the sample point.
		float sample_height = length(sample_position) - PLANET_RADIUS;

		// We sample the medium at the current height.
		Medium_Sample medium_sample = sample_medium(sample_height);

		// From this sample point, we march a ray toward the sun. So we have to repeat the previous process but now
		// from the sample point towards the sun.

		// Compute the distance to integrate towards the sun.
		float sun_t0 = 0.0f;
		float sun_t1 = 0.0f;

		if(intersect_ray_sphere(sample_position, sun_direction, PLANET_RADIUS, sun_t0, sun_t1) && sun_t0 > 0.0f)
		{
			// If the sun hits the ground, we continue.
			continue;
		}

		intersect_ray_sphere(sample_position, sun_direction, ATMOSPHERE_RADIUS, sun_t0, sun_t1);

		// Compute sun transmittance. Eq. 2 `T(sample_point, sun)`.
		// Sample from transmittance LUT.

		float sun_sample_cos_angle = dot(normalize(sample_position), sun_direction);
		float sun_sample_height = sample_height / (ATMOSPHERE_RADIUS - PLANET_RADIUS);
		float2 transmittance_uv = float2(sun_sample_cos_angle * 0.5f + 0.5f, sun_sample_height);

		float3 sun_transmittance = transmittance_lut.Sample(transmittance_uv).rgb;

		// How much light scatters toward the camera at this point.
		float3 in_scattering = (medium_sample.S_s_rayleigh * rayleigh_phase_value
			+ medium_sample.S_s_mie * mie_phase_value)
			* sun_transmittance * transmittance * SUN_INTENSITY;

		// Update luminance.
		luminance += in_scattering * dt;

		// Update transmittance.
		transmittance *= exp(-medium_sample.S_t * dt);
	}

	float sun_disk = step(cos(skybox_ubo.sun_radius), cos_theta);
	luminance += transmittance * SUN_INTENSITY * sun_disk;

	return float4(luminance, 1.0f);
}
