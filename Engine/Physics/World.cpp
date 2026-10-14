// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Physics/World.hpp"

#include "Engine/Core/Math/Color.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <box3d/box3d.h>
#include <math.h>
#include <string.h>

namespace
{
/// Number of segments that approximate a full circle. It must be a multiple of 4 so that the side lines land on
/// segment boundaries and each hemisphere arc gets half of them.
constexpr int CAPSULE_RING_SEGMENT_COUNT = 16;
static_assert(CAPSULE_RING_SEGMENT_COUNT % 4 == 0);

/// Converts a Box3D debug color into a `Color_RGB`.
blk::Color_RGB<float> convert_hex_color(b3HexColor color);

/// Box3D callback to create a debug shape.
void* create_debug_shape(const b3DebugShape* debug_shape, void* context);
/// Box3D callback to destroy a debug shape.
void destroy_debug_shape(void* user_shape, void* context);

/// Box3D callback to draw a shape.
bool draw_shape(void* user_shape, b3WorldTransform transform, b3HexColor color, void* context);
/// Box3D callback to draw a heightfield triangle.
/// @param context A `Triangle_Context`.
bool draw_triangle(b3Vec3 a, b3Vec3 b, b3Vec3 c, int triangle_index, void* context);

/// What `draw_triangle` needs to draw a triangle in world space.
struct Triangle_Context
{
	b3WorldTransform transform;
	blk::Color_RGB<float> color;
};

/// Box3D callback to draw the AABB box.
void draw_bounds(b3AABB aabb, b3HexColor color, void* context);
}  // namespace

blk::Physics
blk::create_physics_world()
{
	b3WorldDef world_def = b3DefaultWorldDef();
	world_def.gravity = {0.0f, -10.0f, 0.0f};
	world_def.createDebugShape = create_debug_shape;
	world_def.destroyDebugShape = destroy_debug_shape;

	Physics physics = {};
	physics.world_id = b3CreateWorld(&world_def);

	return physics;
}

void
blk::destroy_physics_world(Physics& physics)
{
	b3DestroyWorld(physics.world_id);

	physics = {};
}

void
blk::draw_physics_debug(const Physics& physics)
{
	// Set up debug draw callback.

	b3DebugDraw draw = b3DefaultDebugDraw();
	draw.DrawShapeFcn = draw_shape;
	draw.DrawBoundsFcn = draw_bounds;
	draw.drawShapes = true;
	draw.drawBounds = true;

	// Box3D only draws shapes overlapping `drawingBounds`, which defaults to 100 meters around the origin. We draw
	// every shape in the world.
	draw.drawingBounds = b3World_GetBounds(physics.world_id);

	// `UINT64_MAX` means draw shapes for every collision category.
	b3World_Draw(physics.world_id, &draw, UINT64_MAX);
}

namespace
{

blk::Color_RGB<float>
convert_hex_color(const b3HexColor color)
{
	const auto rgb = static_cast<uint32_t>(color);

	return {
		.r = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
		.g = static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
		.b = static_cast<float>(rgb & 0xFF) / 255.0f,
	};
}

void*
create_debug_shape(const b3DebugShape* debug_shape, void* context)
{
	// We are using immediate mode drawing, so we just return a pointer to the Box3D debug shape.

	void* user_shape = nullptr;
	memcpy(&user_shape, &debug_shape->shapeId, sizeof(b3ShapeId));

	return user_shape;
}

void
destroy_debug_shape(void* user_shape, void* context)
{
	// We have nothing to destroy.
}

bool
draw_shape(void* user_shape, const b3WorldTransform transform, const b3HexColor color, void* context)
{
	b3ShapeId shape_id = {};
	memcpy(&shape_id, &user_shape, sizeof(b3ShapeId));

	if (b3Shape_GetType(shape_id) == b3_capsuleShape)
	{
		// Draw capsule.
		b3Capsule capsule = b3Shape_GetCapsule(shape_id);

		// A capsule is a segment from `center1` to `center2`, swept by `radius`.
		// `segment` is the vector along the capsule.
		// `axis` is that vector normalized: the capsule's direction.

		const b3Vec3 segment = b3Sub(capsule.center2, capsule.center1);
		const b3Vec3 axis = b3Normalize(segment);

		// Form a 3D coordinate frame aligned with the capsule.
		const b3Vec3 u = b3Perp(axis);
		const b3Vec3 v = b3Cross(axis, u);

		// Compute line color.
		blk::Color_RGB<float> line_color = convert_hex_color(color);

		// Define lambda to draw each line.
		const auto draw_body_line = [&](const b3Vec3& start, const b3Vec3& end) {
			// Get world positions.

			const b3Pos world_start = b3TransformWorldPoint(transform, start);
			const b3Pos world_end = b3TransformWorldPoint(transform, end);

			// Draw line.
			blk::draw_line(
				{world_start.x, world_start.y, world_start.z},
				{world_end.x, world_end.y, world_end.z},
				line_color
			);
		};

		// Rings and side lines.
		// The following loop draws both capsule rings plus the 4 side lines.

		// Angle between two neighboring points on a ring, in radians.
		constexpr float ring_step = 2.0f * B3_PI / CAPSULE_RING_SEGMENT_COUNT;

		for (int i = 0; i < CAPSULE_RING_SEGMENT_COUNT; ++i)
		{
			// Draw both rings.

			const float angle0 = ring_step * static_cast<float>(i);
			const float angle1 = ring_step * static_cast<float>(i + 1);

			const b3Vec3 offset0 =
				b3Add(b3MulSV(capsule.radius * cosf(angle0), u), b3MulSV(capsule.radius * sinf(angle0), v));
			const b3Vec3 offset1 =
				b3Add(b3MulSV(capsule.radius * cosf(angle1), u), b3MulSV(capsule.radius * sinf(angle1), v));

			draw_body_line(b3Add(capsule.center1, offset0), b3Add(capsule.center1, offset1));
			draw_body_line(b3Add(capsule.center2, offset0), b3Add(capsule.center2, offset1));

			// Draw one side line every quarter turn.

			if (i % (CAPSULE_RING_SEGMENT_COUNT / 4) == 0)
			{
				draw_body_line(b3Add(capsule.center1, offset0), b3Add(capsule.center2, offset0));
			}
		}

		// Hemisphere arcs.
		// The following loop draws the arcs on each hemisphere.

		// Each is a half circle from `-direction` to `+direction` that passes over the pole, in the
		// plane spanned by `direction` and `axis`. Cap 1 bulges away from `center2`, and cap 2 away from `center1`.

		constexpr int arc_segment_count = CAPSULE_RING_SEGMENT_COUNT / 2;
		constexpr float arc_step = B3_PI / arc_segment_count;

		for (const b3Vec3 arc_directions[2] = {u, v}; const b3Vec3& direction : arc_directions)
		{
			for (int i = 0; i < arc_segment_count; ++i)
			{
				const float angle0 = arc_step * static_cast<float>(i);
				const float angle1 = arc_step * static_cast<float>(i + 1);

				const b3Vec3 radial0 = b3MulSV(capsule.radius * cosf(angle0), direction);
				const b3Vec3 radial1 = b3MulSV(capsule.radius * cosf(angle1), direction);
				const b3Vec3 axial0 = b3MulSV(capsule.radius * sinf(angle0), axis);
				const b3Vec3 axial1 = b3MulSV(capsule.radius * sinf(angle1), axis);

				draw_body_line(
					b3Sub(b3Add(capsule.center1, radial0), axial0),
					b3Sub(b3Add(capsule.center1, radial1), axial1)
				);

				draw_body_line(
					b3Add(b3Add(capsule.center2, radial0), axial0),
					b3Add(b3Add(capsule.center2, radial1), axial1)
				);
			}
		}
	}
	else if (b3Shape_GetType(shape_id) == b3_heightShape)
	{
		// Draw heightfield.
		// Querying its whole local bounds visits every triangle.

		// const b3HeightFieldData* height_field = b3Shape_GetHeightField(shape_id);
		// Triangle_Context triangle_context = {
		// 	.transform = transform,
		// 	.color = convert_hex_color(color),
		// };
		//
		// b3QueryHeightField(height_field, height_field->aabb, draw_triangle, &triangle_context);
	}

	return true;
}

bool
draw_triangle(const b3Vec3 a, const b3Vec3 b, const b3Vec3 c, int triangle_index, void* context)
{
	const auto* triangle_context = static_cast<const Triangle_Context*>(context);

	// Triangles are in body space, so we move them to world space.

	const b3Pos world_a = b3TransformWorldPoint(triangle_context->transform, a);
	const b3Pos world_b = b3TransformWorldPoint(triangle_context->transform, b);
	const b3Pos world_c = b3TransformWorldPoint(triangle_context->transform, c);

	// Neighbouring triangles share edges, so those edges are drawn twice.

	blk::draw_line({world_a.x, world_a.y, world_a.z}, {world_b.x, world_b.y, world_b.z}, triangle_context->color);
	blk::draw_line({world_b.x, world_b.y, world_b.z}, {world_c.x, world_c.y, world_c.z}, triangle_context->color);
	blk::draw_line({world_c.x, world_c.y, world_c.z}, {world_a.x, world_a.y, world_a.z}, triangle_context->color);

	// Keep querying.
	return true;
}

void
draw_bounds(const b3AABB aabb, const b3HexColor color, void* context)
{
	const blk::Color_RGB<float> line_color = convert_hex_color(color);

	const b3Vec3& lower = aabb.lowerBound;
	const b3Vec3& upper = aabb.upperBound;

	// Define lambda to get each box corner.
	// Each bit of a corner index picks the lower or upper bound on one axis: bit 0 for x, bit 1 for y and bit 2 for z.
	// Two corners share an edge when their indices differ in exactly one bit.
	const auto get_corner = [&](const uint32_t index) -> blk::Vector3 {
		return {
			index & 1 ? upper.x : lower.x,
			index & 2 ? upper.y : lower.y,
			index & 4 ? upper.z : lower.z,
		};
	};

	for (uint32_t corner_index = 0; corner_index < 8; ++corner_index)
	{
		for (uint32_t axis_bit = 1; axis_bit <= 4; axis_bit <<= 1)
		{
			if (!(corner_index & axis_bit))
			{
				blk::draw_line(get_corner(corner_index), get_corner(corner_index | axis_bit), line_color);
			}
		}
	}
}
}  // namespace
