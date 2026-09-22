// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Library/Math.slang"

/// Contants of Sébastien Hillaire's model.

// Define coefficients of the different participating media components constituting the Earth's atmosphere.
// The original model uses meters, we use kilometers to avoid precision loss when precomputing LUTs.

static const float3 RAYLEIGH_SCATTERING = float3(5.802f, 13.558f, 33.1f) * 1e-3f;
static const float MIE_SCATTERING = 3.996e-3f;
static const float MIE_ABSORPTION = 4.40e-3f;
static const float3 OZONE_ABSORPTION = float3(0.650f, 1.881f, 0.085f) * 1e-3f;

// Define model scales.

// Every `x` km up, density drops by a factor of `e`.
static const float RAYLEIGH_SCALE = 8.0f;
// Every `x` km, aerosols drop by a factor of `e`.
static const float MIE_SCALE = 1.2f;
// Ozone is not dragged down by gravity.
// 1 at the mid point, falling linearly to 0 at the low point.
static const float OZONE_MID_POINT = 25.0f;
static const float OZONE_LOW_POINT = 15.0f;
/// Radius of the planet in kilometers.
static const float PLANET_RADIUS = 6360.0f;
/// Radius of the atmosphere in kilometers.
static const float ATMOSPHERE_RADIUS = 6460.0f;
/// Minimum view height in kilometers.
static const float MINIMUM_VIEW_HEIGHT = 0.001f;

/// Asymmetry parameter of the Henyey-Greenstein phase function used for aerosols.
static const float MIE_ASYMMETRY = 0.8f;

/// Intensity of the sun.
static const float3 SUN_INTENSITY = float3(10.0f);

/// Albedo of the planet ground, a uniform purely diffuse response. Section 4.
static const float3 GROUND_ALBEDO = float3(0.3f);

// A photon interacts with the medium (atmosphere). The atmosphere is composed of air molecules (Rayleigh),
// aerosols (Mie), and ozone. The density of these components varies depending on altitude.
//
// S_a: per-wavelength absorption.
// S_s: scattering.
// S_t = S_a + S_s: the extintion coefficient. It represents the total amount of energy lost due to absorption and
// out-scattering.
struct Medium_Sample
{
	float d_rayleigh;
	float d_mie;
	float d_ozone;

	float3 S_s_rayleigh;
	float3 S_s_mie ;

	float3 S_a_mie;
	float3 S_a_ozone;

	float3 S_t;
};

/// Samples the medium (atmosphere) at a given height.
Medium_Sample sample_medium(float sample_height);

// The phase functions describe the directional preference of scattering. When light hits a particle and scatters, it
// does not scatter equally in all directions. They only depend on the angle between the sun and the view direction.

/// Rayleigh phase function, for scattering off air molecules.
/// @param cos_theta Cosine of the angle between the view direction and the sun direction.
float rayleigh_phase(float cos_theta);
/// Mie phase function (Henyey-Greenstein), for scattering off aerosols.
/// @param cos_theta Cosine of the angle between the view direction and the sun direction.
float mie_phase(float cos_theta);

/// Isotropic phase function `p_u`, Section 4. Scattering orders past the first are taken to have no preferred
/// direction, so the multiple scattering approximation uses this one instead of the two above. Section 5.5.1.
static const float UNIFORM_PHASE = 1.0f / (4.0f * float(PI));

// The transmittance LUT stores `T(x, x_o)`, the transmittance from a point to the atmosphere boundary. By spherical
// symmetry it depends only on the radius `r` from the planet center and the cosine `mu` of the angle between the ray
// and the zenith. Section 5.2, which takes its representation from Section 4 of Bruneton and Neyret.
//
// Mapping `mu` linearly wastes the table. The distance a ray travels barely changes over most of the upper
// hemisphere, then grows enormously in the last fraction of a degree before the horizon. Bruneton parameterizes by
// distance instead, with
//
//     rho = sqrt(r^2 - PLANET_RADIUS^2)                    distance from the view to its own horizon
//     H   = sqrt(ATMOSPHERE_RADIUS^2 - PLANET_RADIUS^2)    the same distance from the ground
//
// `u_r` is `rho / H`, and `u_mu` spans the travelled distance from the shortest ray (straight up) to the longest
// (grazing the horizon). Bruneton's second branch, for rays meeting the ground, is dropped: one stopped by the planet
// carries no sunlight, so every texel holds a ray reaching the boundary and `u_mu = 1` is exactly the horizon.

/// Converts transmittance LUT texture coordinates to the physical parameters they stand for.
/// @param uv Texture coordinates of the texel center, `(thread_id + 0.5f) / lut_size`.
/// @param lut_size Dimensions of the transmittance LUT in texels.
/// @param radius Distance from the planet center in kilometers.
/// @param cos_angle Cosine of the angle between the ray and the zenith.
void transmittance_uv_to_parameters(float2 uv, float2 lut_size, out float radius, out float cos_angle);

/// The inverse of `transmittance_uv_to_parameters`.
/// @param radius Distance from the planet center in kilometers.
/// @param cos_angle Cosine of the angle between the ray and the zenith.
/// @param lut_size Dimensions of the transmittance LUT in texels.
float2 transmittance_parameters_to_uv(float radius, float cos_angle, float2 lut_size);

// The sky-view LUT is a latitude/longitude image of the distant sky, oriented with respect to the view's local up
// vector so the horizon is always the horizontal line across its middle. Section 5.3.
//
// Latitude uses the paper's non-linear mapping, packing texels near the horizon where the sky has the most detail:
//
//     v = 0.5 + 0.5 * sign(l) * sqrt(|l| / (PI / 2)), with l in [-PI / 2, PI / 2]
//
// Latitude 0 is the horizontal plane through the view, not the true horizon, which dips below it by
// `acos(PLANET_RADIUS / (PLANET_RADIUS + view_height))`. They agree near the ground and separate as the view climbs.
//
// Longitude is the azimuth from the sun. The atmosphere is symmetric about the sun/zenith plane, so `u` stores
// half a turn instead of a full one, which doubles the horizontal resolution and leaves no seam to wrap around —
// hence the clamp to edge sampler.
//
// The two functions below are inverses. `CS_Skybox_Sky_View.slang` writes through the first and `FS_Skybox.slang`
// reads through the second, so both must agree for the horizon to land where the fragment shader looks for it.

/// Shortest horizontal projection `sky_view_direction_to_uv` still measures a longitude from.
static const float SKY_VIEW_MINIMUM_HORIZONTAL_LENGTH = 1e-6f;

/// Converts a sky-view LUT texel center to the view direction it stores, in the canonical frame: up is +Y, as in
/// world space, and the sun's horizontal projection is rotated onto +X. Everything derived from the direction stays
/// in that frame, so the caller must take the sun from `sky_view_canonical_sun_direction`.
/// @param uv Texture coordinates of the texel center, `(thread_id + 0.5f) / lut_size`.
/// @param lut_size Dimensions of the sky-view LUT in texels.
float3 sky_view_uv_to_direction(float2 uv, float2 lut_size);

/// Returns the sun direction in the canonical frame `sky_view_uv_to_direction` works in.
/// @param sun_direction Normalized direction toward the sun in world space.
float3 sky_view_canonical_sun_direction(float3 sun_direction);

/// The inverse of `sky_view_uv_to_direction`. Only the angle between the two horizontal projections reaches the LUT,
/// and world space shares its up vector with the canonical frame, so neither direction has to be rotated onto it.
/// @param direction Normalized view direction in world space.
/// @param sun_direction Normalized direction toward the sun in world space.
/// @param lut_size Dimensions of the sky-view LUT in texels.
float2 sky_view_direction_to_uv(float3 direction, float3 sun_direction, float2 lut_size);

// The multiscattering LUT stores `psi_ms`, the transfer function of Eq. 10: multiplied by a directional light's
// illuminance it gives the luminance reaching a point from every scattering order past the first. Section 5.5.2,
// whose parameterization is
//
//     u = 0.5 + 0.5 * cos(sun_zenith_angle)
//     v = height / (ATMOSPHERE_RADIUS - PLANET_RADIUS)
//
// Both axes map linearly, unlike the two LUTs above: the stored function is broadly uniform over the whole table and
// has no horizon to resolve. It describes the atmosphere alone, so it holds for any view position and sun direction
// and only goes stale when the material setup changes.

/// Converts a multiscattering LUT texel center to the physical parameters it stands for.
/// @param uv Texture coordinates of the texel center, `(thread_id + 0.5f) / lut_size`.
/// @param lut_size Dimensions of the multiscattering LUT in texels.
/// @param height Altitude above the planet surface in kilometers.
/// @param cos_sun_zenith_angle Cosine of the angle between the sun direction and the zenith.
void multiscattering_uv_to_parameters(float2 uv, float2 lut_size, out float height, out float cos_sun_zenith_angle);

/// The inverse of `multiscattering_uv_to_parameters`.
/// @param height Altitude above the planet surface in kilometers.
/// @param cos_sun_zenith_angle Cosine of the angle between the sun direction and the zenith.
/// @param lut_size Dimensions of the multiscattering LUT in texels.
float2 multiscattering_parameters_to_uv(float height, float cos_sun_zenith_angle, float2 lut_size);

Medium_Sample
sample_medium(float sample_height)
{
	Medium_Sample medium = {};

	medium.d_rayleigh = exp(-sample_height / RAYLEIGH_SCALE);
	medium.d_mie = exp(-sample_height / MIE_SCALE);
	medium.d_ozone = saturate(1.0f - abs(sample_height - OZONE_MID_POINT) / OZONE_LOW_POINT);

	// Compute `S_t` components.

	// float3 S_a_rayleigh = RAYLEIGH_ABSORPTION * d_rayleigh;
	medium.S_s_rayleigh = RAYLEIGH_SCATTERING * medium.d_rayleigh;

	medium.S_a_mie = MIE_ABSORPTION * medium.d_mie;
	medium.S_s_mie = MIE_SCATTERING * medium.d_mie;

	medium.S_a_ozone = OZONE_ABSORPTION * medium.d_ozone;
	// medium.S_s_ozone = OZONE_SCATTERING * mediumn.d_ozone;

	medium.S_t = medium.S_s_rayleigh + medium.S_a_mie + medium.S_s_mie + medium.S_a_ozone;

	return medium;
}

float
rayleigh_phase(float cos_theta)
{
	return 3.0f / (16.0f * float(PI)) * (1.0f + cos_theta * cos_theta);
}

float
mie_phase(float cos_theta)
{
	float g = MIE_ASYMMETRY;

	return 3.0f / (8.0f * float(PI))
		* ((1.0f - g * g) * (1.0f + cos_theta * cos_theta))
		/ ((2.0f + g * g) * pow(1.0f + g * g - 2.0f * g * cos_theta, 1.5f));
}

void
transmittance_uv_to_parameters(float2 uv, float2 lut_size, out float radius, out float cos_angle)
{
	// Spread the texel centers over the whole [0, 1] range, as the sky-view pair does.
	float2 unit = (uv * lut_size - 0.5f) / (lut_size - 1.0f);

	float H = sqrt(ATMOSPHERE_RADIUS * ATMOSPHERE_RADIUS - PLANET_RADIUS * PLANET_RADIUS);

	// `u_r = rho / H`, inverted. The radius follows from the horizon distance because the view, its horizon and the
	// planet center form a right angle at the horizon.
	float rho = H * unit.y;
	radius = sqrt(rho * rho + PLANET_RADIUS * PLANET_RADIUS);

	// Distance travelled to the atmosphere boundary, spanning the range this radius allows.
	float d_min = ATMOSPHERE_RADIUS - radius;
	float d_max = rho + H;
	float d = d_min + unit.x * (d_max - d_min);

	// Recover the zenith cosine by inverting the intersection that produced `d`:
	// `radius^2 + 2 * radius * cos_angle * d + d^2 = ATMOSPHERE_RADIUS^2`, where `ATMOSPHERE_RADIUS^2 - radius^2`
	// is `H^2 - rho^2`. A view on the boundary looking straight up travels nowhere, so guard the division.
	cos_angle = d == 0.0f ? 1.0f : (H * H - rho * rho - d * d) / (2.0f * radius * d);
	cos_angle = clamp(cos_angle, -1.0f, 1.0f);
}

float2
transmittance_parameters_to_uv(float radius, float cos_angle, float2 lut_size)
{
	float H = sqrt(max(0.0f, ATMOSPHERE_RADIUS * ATMOSPHERE_RADIUS - PLANET_RADIUS * PLANET_RADIUS));
	float rho = sqrt(max(0.0f, radius * radius - PLANET_RADIUS * PLANET_RADIUS));

	// Distance to the atmosphere boundary, the far root of the ray against it. The view is inside the atmosphere, so
	// the near root is behind it and only the far one is travelled.
	float discriminant = radius * radius * (cos_angle * cos_angle - 1.0f) + ATMOSPHERE_RADIUS * ATMOSPHERE_RADIUS;
	float d = max(0.0f, -radius * cos_angle + sqrt(max(0.0f, discriminant)));

	float d_min = ATMOSPHERE_RADIUS - radius;
	float d_max = rho + H;

	float unit_u = (d - d_min) / (d_max - d_min);
	float unit_v = rho / H;

	// Map back onto the texel centers `transmittance_uv_to_parameters` spread over [0, 1].
	return (float2(unit_u, unit_v) * (lut_size - 1.0f) + 0.5f) / lut_size;
}

float3
sky_view_uv_to_direction(float2 uv, float2 lut_size)
{
	// Spread the texel centers over the whole [0, 1] range. A texel center sits half a texel in from each edge, so
	// without this the LUT never stores the zenith or the nadir themselves and the sampler has nothing but the
	// outermost texel to clamp to once the view direction passes it.
	float2 unit = (uv * lut_size - 0.5f) / (lut_size - 1.0f);

	// Undo the non-linear latitude mapping. Squaring the signed distance from the horizon inverts the paper's square
	// root, and `sign` carries it back to the hemisphere it came from.
	float horizon_distance = unit.y * 2.0f - 1.0f;
	float latitude = sign(horizon_distance) * horizon_distance * horizon_distance * (float(PI) / 2.0f);

	// Longitude is measured from the sun, which the canonical frame puts on +X.
	float longitude = unit.x * float(PI);

	// Latitude is measured from the horizon rather than from the zenith, so it is the sine that gives the vertical
	// component and the cosine that scales the horizontal one.
	float cos_latitude = cos(latitude);

	return float3(cos_latitude * cos(longitude), sin(latitude), cos_latitude * sin(longitude));
}

float3
sky_view_canonical_sun_direction(float3 sun_direction)
{
	// Rotating the sun's horizontal projection onto +X leaves its zenith angle untouched, so the canonical direction
	// only has to put the horizontal part back at full length along +X.
	float cos_zenith = clamp(sun_direction.y, -1.0f, 1.0f);
	float sin_zenith = sqrt(1.0f - cos_zenith * cos_zenith);

	return float3(sin_zenith, cos_zenith, 0.0f);
}

float2
sky_view_direction_to_uv(float3 direction, float3 sun_direction, float2 lut_size)
{
	// Latitude is measured from the horizon, so it comes straight from the component along the planet up vector.
	float latitude = asin(clamp(direction.y, -1.0f, 1.0f));

	// The paper's non-linear latitude mapping. Section 5.3.
	float unit_v = 0.5f + 0.5f * sign(latitude) * sqrt(abs(latitude) / (float(PI) / 2.0f));

	// Longitude is the angle between the horizontal projections of the view and the sun. Looking straight up or down
	// leaves nothing to project, and neither does a sun at the zenith. The longitude is meaningless in both cases, so
	// it falls back to zero instead of dividing by a length of zero.
	float direction_horizontal_length = length(direction.xz);
	float sun_horizontal_length = length(sun_direction.xz);

	float unit_u = 0.0f;

	if (direction_horizontal_length > SKY_VIEW_MINIMUM_HORIZONTAL_LENGTH
		&& sun_horizontal_length > SKY_VIEW_MINIMUM_HORIZONTAL_LENGTH)
	{
		float cos_longitude =
			dot(direction.xz / direction_horizontal_length, sun_direction.xz / sun_horizontal_length);

		unit_u = acos(clamp(cos_longitude, -1.0f, 1.0f)) / float(PI);
	}

	// Map back onto the texel centers `sky_view_uv_to_direction` spread over [0, 1].
	return (float2(unit_u, unit_v) * (lut_size - 1.0f) + 0.5f) / lut_size;
}

void
multiscattering_uv_to_parameters(float2 uv, float2 lut_size, out float height, out float cos_sun_zenith_angle)
{
	// Spread the texel centers over the whole [0, 1] range, as the other two LUTs do.
	float2 unit = (uv * lut_size - 0.5f) / (lut_size - 1.0f);

	cos_sun_zenith_angle = clamp(unit.x * 2.0f - 1.0f, -1.0f, 1.0f);
	height = unit.y * (ATMOSPHERE_RADIUS - PLANET_RADIUS);
}

float2
multiscattering_parameters_to_uv(float height, float cos_sun_zenith_angle, float2 lut_size)
{
	float unit_u = saturate(0.5f + 0.5f * cos_sun_zenith_angle);
	float unit_v = saturate(height / (ATMOSPHERE_RADIUS - PLANET_RADIUS));

	// Map back onto the texel centers `multiscattering_uv_to_parameters` spread over [0, 1].
	return (float2(unit_u, unit_v) * (lut_size - 1.0f) + 0.5f) / lut_size;
}
