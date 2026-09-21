// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Global_Set.slang"
#include "Library/Atmosphere.slang"
#include "Library/Math.slang"

/// Step between samples along the ray (view -> sample_point).
static const int STEP_COUNT = 40;

/// Computes the transmittance LUT of the Sébastien Hillaire's model. This is done only once at renderer
/// initialization and the image is read-only after that. This image is then sampled by `FS_Skybox.slang`.
[shader("compute")]
[numthreads(8,8,1)]
void
cs_main(uint3 thread_id : SV_DispatchThreadID)
{
	// First, we need to convert the `thread_id` to physical parameters.

	// Get LUT dimensions.
	uint lut_width = 0;
	uint lut_height = 0;
	transmittance_storage_image.GetDimensions(lut_width, lut_height);

	// Convert `thread_id` to UV in [0, 1];
	float2 uv = (float2(thread_id.xy) + 0.5f) / float2(lut_width, lut_height);

	// Map UV to physical parameters.
	// u (sun zenith cosine): -1 below horizon) to 1 (straight up).
	// v (altitude): 0 (ground) to atmosphere thickness.

	float cos_angle = uv.x * 2.0f - 1.0f;
	float height = uv.y * (ATMOSPHERE_RADIUS -  PLANET_RADIUS);

	// Build the starting position.
	float3 origin = float3(0.0f, PLANET_RADIUS + height, 0.0f);

	// Build the direction from the zenith cosine.
	float sin_angle = sqrt(1.0f - cos_angle * cos_angle);
	float3 direction = float3(sin_angle, cos_angle, 0.0f);

	// Compute the distance to integrate: find where the ray exits the atmosphere.

	float t0 = 0.0f;
	float t1 = 0.0f;

	// The ray always starts inside the atmosphere for now, so `t0` is behind us and the exit point `t1 ` is the far end
	// of the segment we have to integrate over.

	if (!intersect_ray_sphere(origin, direction, ATMOSPHERE_RADIUS, t0, t1))
	{
		// There is no atmosphere, so nothing to integrate.
		transmittance_storage_image[thread_id.xy] = float4(1.0f);

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

	// March ray and accumulate optical depth.

	float dt = t_max / float(STEP_COUNT);
	float3 optical_depth = float3(0.0f);

	for (int i = 0; i < STEP_COUNT; ++i)
	{
		// Sample at the midpoint of each segment.
		float t = dt * (float(i) + 0.5f);

		float3 sample_position = origin + direction * t;
		float sample_height = length(sample_position) - PLANET_RADIUS;

		Medium_Sample medium_sample = sample_medium(sample_height);
		optical_depth += medium_sample.S_t * dt;
	}

	transmittance_storage_image[thread_id.xy] = float4(exp(-optical_depth), 1.0f);
}
