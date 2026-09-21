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
// static const float RAYLEIGH_ABSORPTION = 0;
static const float MIE_SCATTERING = 3.996e-3f;
static const float MIE_ABSORPTION = 4.40e-3f;
// static const float OZONE_SCATTERING = 0;
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
