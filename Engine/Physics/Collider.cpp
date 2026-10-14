// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Physics/Collider.hpp"

#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Physics/Body.hpp"
#include "Engine/Platform/Assert.hpp"

namespace
{
/// Creates a capsule collider for any body.
blk::Capsule_Collider create_capsule_collider(b3BodyId body_id);
}  // namespace

blk::Capsule_Collider
blk::create_capsule_collider(const Static_Body& body)
{
	return ::create_capsule_collider(body.id);
}

blk::Capsule_Collider
blk::create_capsule_collider(const Dynamic_Body& body)
{
	return ::create_capsule_collider(body.id);
}

void
blk::destroy_capsule_collider(Capsule_Collider& collider)
{
}

blk::Heightfield_Collider
blk::create_heightfield_collider(const Static_Body& body, float* samples, uint32_t count, float height, float spacing)
{
	b3HeightFieldDef heightfield_def = {};
	heightfield_def.heights = samples;
	heightfield_def.countX = static_cast<int>(count);
	heightfield_def.countZ = static_cast<int>(count);
	heightfield_def.scale = {spacing, 1.0f, spacing};
	heightfield_def.globalMinimumHeight = 0.0f;
	heightfield_def.globalMaximumHeight = height;

	const b3ShapeDef shape_def = b3DefaultShapeDef();

	Heightfield_Collider collider = {};
	collider.data = b3CreateHeightField(&heightfield_def);
	collider.id = b3CreateHeightFieldShape(body.id, &shape_def, collider.data);

	return collider;
}

void
blk::destroy_heightfield_collider(Heightfield_Collider& collider)
{
	// The shape references `data`, so it must be destroyed first. Destroying its body destroys it too.
	if (b3Shape_IsValid(collider.id))
	{
		b3DestroyShape(collider.id, false);
	}

	if (collider.data)
	{
		b3DestroyHeightField(collider.data);
	}

	collider = {};
}

void
blk::set_collider_transform(const Capsule_Collider& collider, const Matrix4& local_matrix)
{
	if (!BLK_VERIFY(B3_IS_NON_NULL(collider.id)))
	{
		return;
	}

	const Vector4 center1 =
		local_matrix *
		Vector4{collider.capsule.center1.x, collider.capsule.center1.y, collider.capsule.center1.z, 1.0f};
	const Vector4 center2 =
		local_matrix *
		Vector4{collider.capsule.center2.x, collider.capsule.center2.y, collider.capsule.center2.z, 1.0f};

	const b3Capsule capsule = {
		.center1 = {center1.x, center1.y, center1.z},
		.center2 = {center2.x, center2.y, center2.z},
		.radius = collider.capsule.radius,
	};

	b3Shape_SetCapsule(collider.id, &capsule);
}

namespace
{
blk::Capsule_Collider
create_capsule_collider(const b3BodyId body_id)
{
	blk::Capsule_Collider collider = {};
	collider.capsule.center1 = {0.0f, -1.0f, 0.0f};
	collider.capsule.center2 = {0.0f, 1.0f, 0.0f};
	collider.capsule.radius = 0.25f;

	b3ShapeDef shape_def = b3DefaultShapeDef();
	shape_def.density = 1.0f;
	shape_def.baseMaterial.friction = 0.3f;

	collider.id = b3CreateCapsuleShape(body_id, &shape_def, &collider.capsule);

	return collider;
}
}  // namespace
