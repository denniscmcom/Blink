// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Physics/Body.hpp"

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Platform/Assert.hpp"
#include "World.hpp"

#include <box3d/box3d.h>

blk::Static_Body
blk::create_static_body(const Physics& physics, const Vector3& position)
{
	b3BodyDef body_def = b3DefaultBodyDef();
	body_def.position = {position.x, position.y, position.z};

	return {
		.id = b3CreateBody(physics.world_id, &body_def),
	};
}

void
blk::destroy_static_body(Static_Body& body)
{
	b3DestroyBody(body.id);

	body = {};
}

blk::Dynamic_Body
blk::create_dynamic_body(const Physics& physics, const Vector3& position)
{
	b3BodyDef body_def = b3DefaultBodyDef();
	body_def.type = b3_dynamicBody;
	body_def.position = {position.x, position.y, position.z};

	return {
		.id = b3CreateBody(physics.world_id, &body_def),
	};
}

void
blk::destroy_dynamic_body(Dynamic_Body& body)
{
	b3DestroyBody(body.id);

	body = {};
}

void
blk::set_body_transform(const Static_Body& body, const Matrix4& world_matrix)
{
	if (!BLK_VERIFY(B3_IS_NON_NULL(body.id)))
	{
		return;
	}

	const b3Matrix3 rotation_matrix = {
		.cx = b3Normalize({world_matrix.columns[0].x, world_matrix.columns[0].y, world_matrix.columns[0].z}),
		.cy = b3Normalize({world_matrix.columns[1].x, world_matrix.columns[1].y, world_matrix.columns[1].z}),
		.cz = b3Normalize({world_matrix.columns[2].x, world_matrix.columns[2].y, world_matrix.columns[2].z}),
	};

	const b3Pos position = {
		.x = world_matrix[3].x,
		.y = world_matrix[3].y,
		.z = world_matrix[3].z,
	};

	b3Body_SetTransform(body.id, position, b3MakeQuatFromMatrix(&rotation_matrix));
}
