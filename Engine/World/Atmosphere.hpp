// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Color.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Core/Pool.hpp"

namespace blk
{
struct Node;

/// Atmosphere & skybox settings.
/// Defaults are the Earth-like constants of Sébastien Hillaire's model, converted from meters to kilometers.
struct Atmosphere_Settings
{
	/// The `Node_Type::DIRECTIONAL_LIGHT` node used as the sun. The skybox is not rendered without one.
	/// The `x` scale component of the node is used as the sun radius in degrees.
	Pool_Handle<Node> sun_node_handle;
	/// Rayleigh scattering coefficient per kilometer.
	Vector3 rayleigh_scattering = {.x = 0.005802f, .y = 0.013558f, .z = 0.0331f};
	/// Mie scattering coefficient per kilometer.
	float mie_scattering = 0.003996f;
	/// Mie absorption coefficient per kilometer.
	float mie_absorption = 0.0044f;
	/// Ozone absorption coefficient per kilometer.
	Vector3 ozone_absorption = {.x = 0.00065f, .y = 0.001881f, .z = 0.000085f};
	/// Altitude in kilometers over which Rayleigh density drops by a factor of `e`.
	float rayleigh_scale = 8.0f;
	/// Altitude in kilometers over which Mie density drops by a factor of `e`.
	float mie_scale = 1.2f;
	/// Altitude in kilometers where ozone density peaks at 1.
	float ozone_mid_point = 25.0f;
	/// Distance in kilometers from `ozone_mid_point` over which ozone density falls linearly to 0.
	float ozone_low_point = 15.0f;
	/// Radius of the planet in kilometers.
	float planet_radius = 6360.0f;
	/// Radius of the atmosphere in kilometers.
	float atmosphere_radius = 6460.0f;
	/// Asymmetry parameter of the Henyey-Greenstein phase function used for aerosols.
	float mie_asymmetry = 0.8f;

	/// Albedo of the planet ground, a uniform purely diffuse response, in sRGB like every color picked in the editor.
	/// The default is the sRGB encoding of the model's linear 0.3.
	Color_RGB<float> ground_albedo = {.r = 0.584f, .g = 0.584f, .b = 0.584f};

	/// Enable skybox transmittance rendering pass.
	bool enable_transmittance = true;
	/// Enable skybox multiscattering rendering pass.
	bool enable_multiscattering = true;
	/// Enable skybox sky-view rendering pass.
	bool enable_sky_view = true;
	/// Enable skybox aerial perspective pass.
	bool enable_aerial = true;
};
}  // namespace blk
