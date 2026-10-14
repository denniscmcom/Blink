// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Core.hpp"
#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Unit.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Resource/Mesh.hpp"

namespace blk
{
struct Texture;
struct Material;
struct Mesh;

/// Data that describes a render frame.
struct Frame_Descriptor
{
	// Camera.

	/// Camera view point.
	Vector3 view_position;
	/// Camera view matrix.
	Matrix4 view;
	/// Camera projection matrix.
	Matrix4 projection;

	// Atmosphere and skybox.

	/// Compass direction along the horizon.
	Radians sun_azimuth;
	/// Measured up from the horizon.
	Radians sun_elevation;
	/// Radius of the sun disk.
	Radians sun_radius;
	/// Intensity of the sun light.
	float sun_intensity;

	/// Base color of earth.
	Vector3 ground_albedo;

	/// Should be less than `atmosphere_radius`.
	Radians planet_radius;
	/// Should be greater than `planet_radius`.
	Radians atmosphere_radius;

	Vector3 rayleigh_scattering;
	float rayleigh_scale;

	Vector3 ozone_absorption;
	float ozone_mid_point;
	float ozone_low_point;

	float mie_scattering;
	float mie_absorption;
	float mie_asymmetry;
	float mie_scale;

	// Terrain.

	/// Handle to a terrain heightmap.
	Pool_Handle<Texture> terrain_heightmap_handle;
	/// Size of each terrain chunk in meters.
	float terrain_chunk_size;
	/// Height of the terrain in meters.
	float terrain_height;
	/// Depth in meters of the skirt hanging below each chunk's edges.
	float skirt_depth;

	// Debug flags.

#ifdef BLK_TARGET_DEBUG
	/// Render mesh triangles.
	bool is_debug_triangle_enabled;
	/// Skybox transmittance LUT.
	bool is_skybox_transmittance_enabled;
	/// Skybox multiscattering LUT.
	bool is_skybox_multiscattering_enabled;
	/// Skybox Sky-view LUT.
	bool is_sky_view_enabled;
	/// Skybox aerial LUT.
	bool is_skybox_aerial_enabled;
#endif
};

/// Data that describes a mesh to render.
struct Mesh_Descriptor
{
	/// Mesh world matrix.
	Matrix4 world_matrix;
	/// Handle to a mesh.
	Pool_Handle<Mesh> mesh_handle;
	/// Handle to a material.
	Pool_Handle<Material> material_handle;
};

/// Data that describes a mesh to render.
struct Terrain_Chunk_Descriptor
{
	/// Chunk world matrix.
	Matrix4 world_matrix;
	/// Handle to a flat mesh grid.
	Pool_Handle<Mesh> grid_handle;
	/// Handle to a material.
	Pool_Handle<Material> material_handle;
	/// Corner offset of the heightmap region this chunk covers.
	Vector2 uv_offset;
	/// Side length of the heightmap region this chunk covers.
	float uv_scale;
	/// Chunk LOD.
	uint32_t lod;
};

/// Creates the renderer.
Result create_renderer(size_t width, size_t height);
/// Destroys the renderer.
void destroy_renderer();

/// Waits until the device has finished every submitted command.
void wait_for_device();

/// Begins a new render frame.
void begin_render_frame(const Frame_Descriptor& descriptor);
/// Ends the render frame.
void end_render_frame();

/// Draws a mesh into the current frame.
void draw_mesh(const Mesh_Descriptor& descriptor);
/// Draws a terrain chunk into the current frame.
void draw_terrain_chunk(const Terrain_Chunk_Descriptor& descriptor);
/// Draws a line from `start` to `end`.
void draw_line(const Vector3& start, const Vector3& end, const Color_RGB<float>& color);
}  // namespace blk
