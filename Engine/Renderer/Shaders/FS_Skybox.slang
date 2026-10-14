// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Global_Set.slang"
#include "Library/Atmosphere.slang"
#include "Library/Math.slang"
#include "Pass/Skybox.slang"

/// Renders a dynamic skybox using Sébastien Hillaire's model – A Scalable and Production Ready Sky and Atmosphere
/// Rendering Technique.
/// https://sebh.github.io/publications/egsr2020.pdf
///
/// The sky is a lookup into the sky-view LUT that `CS_Skybox_Sky_View.slang` computed at the start of the frame, so no
/// ray marching happens here. Only the sun disk is evaluated per pixel, because the LUT is too coarse to hold it.
/// Section 5.3.
[shader("fragment")] float4
fs_main(VS_Skybox_Output input) : SV_Target
{
	// Compute view direction.
	float3 direction = normalize(input.direction);

	// Compute sun direction.
	float3 sun_direction = normalize(skybox_ubo.sun_direction);

	// Read the sky out of the LUT.

	uint lut_width = 0;
	uint lut_height = 0;
	skybox_sky_view_lut.GetDimensions(lut_width, lut_height);

	float2 sky_view_uv = sky_view_direction_to_uv(direction, sun_direction, float2(lut_width, lut_height));

	float3 luminance = skybox_sky_view_lut.Sample(sky_view_uv).rgb;

	// Composite the sun disk over it.

	// Angular distance from the sun center, measured as the chord between the two unit vectors. For small angles it
	// matches the angle, and it keeps precision where cos(theta) is too close to 1 to resolve the edge.
	float sun_distance = length(direction - sun_direction);
	float sun_chord_radius = 2.0f * sin(skybox_ubo.sun_radius * 0.5f);

	// Fade the edge over one pixel footprint instead of cutting it hard. The derivative is taken before the branch,
	// where the whole quad is still active.
	float sun_coverage = saturate((sun_chord_radius - sun_distance) / max(fwidth(sun_distance), 1e-6f) + 0.5f);

	if (sun_coverage > 0.0f)
	{
		// Convert view height to kilometers.
		// When view height is close to 0 (on the planet surface), artifacts may happen.
		float view_height = max(skybox_ubo.view_height / 1000.0f, MINIMUM_VIEW_HEIGHT);

		// Compute view position in planet space.
		float3 view_position = float3(0.0f, skybox_ubo.planet_radius + view_height, 0.0f);

		float t0 = 0.0f;
		float t1 = 0.0f;

		// The planet hides the sun whenever the ray toward it reaches the ground first. Without this the disk stays
		// visible through the ground after it has set, since the atmosphere alone barely dims it over the short
		// distance to the surface.
		bool hits_ground =
			intersect_ray_sphere(view_position, direction, skybox_ubo.planet_radius, t0, t1) && t0 > 0.0f;

		if (!hits_ground)
		{
			uint transmittance_lut_width = 0;
			uint transmittance_lut_height = 0;
			skybox_transmittance_lut.GetDimensions(transmittance_lut_width, transmittance_lut_height);

			// The view sits on the planet's up axis, so the zenith cosine of the ray is its own Y component.
			float2 transmittance_uv = transmittance_parameters_to_uv(
				skybox_ubo.planet_radius + view_height,
				direction.y,
				float2(transmittance_lut_width, transmittance_lut_height),
				skybox_ubo.planet_radius,
				skybox_ubo.atmosphere_radius
			);

			// The level is named because only part of a quad reaches this branch, which leaves the derivatives an
			// implicit lookup would need undefined. The LUT has a single level, so it samples the same texels anyway.
			float3 sun_luminance =
				skybox_transmittance_lut.SampleLevel(transmittance_uv, 0.0f).rgb * skybox_ubo.sun_intensity;

			// The output is clamped to [0, 1] with no tone mapping, and the sun is far brighter than 1, so scaling it
			// by coverage would still clamp to white. Blending against the clamped result keeps the edge in range.
			luminance = lerp(luminance, saturate(luminance + sun_luminance), sun_coverage);
		}
	}

	return float4(luminance, 1.0f);
}
