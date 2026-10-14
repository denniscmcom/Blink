// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <box3d/box3d.h>

namespace blk
{
struct Matrix4;
struct Dynamic_Body;
struct Static_Body;

struct Capsule_Collider
{
	b3ShapeId id;
	b3Capsule capsule;
};

/// Creates a capsule collider for a static body.
Capsule_Collider create_capsule_collider(const Static_Body& body);
/// Creates a capsule collider for a dynamic body.
Capsule_Collider create_capsule_collider(const Dynamic_Body& body);
/// Destroys a capsule collider.
void destroy_capsule_collider(Capsule_Collider& collider);

struct Heightfield_Collider
{
	b3HeightFieldData* data;
	b3ShapeId id;
};

/// Creates a heightfield collider.
/// @param body Heightfield body.
/// @param samples Array of height samples of size `count` * `count`.
/// @param count Number of grid lines along the terrain, e.g resolution + 1.
/// @param height Height in meters.
/// @param spacing Distance in meters between two grid lines.
/// @note Only squared heightfields are supported.
Heightfield_Collider create_heightfield_collider(
	const Static_Body& body,
	float* samples,
	uint32_t count,
	float height,
	float spacing
);
/// Destroys the heightfield collider. Its shape may already be gone with its body, but its data is always freed here.
void destroy_heightfield_collider(Heightfield_Collider& collider);

/// Sets a collider transform relative to its parent body.
void set_collider_transform(const Capsule_Collider& collider, const Matrix4& local_matrix);
}  // namespace blk
