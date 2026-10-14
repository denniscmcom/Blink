// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <box3d/box3d.h>

namespace blk
{
struct Matrix4;
struct Physics;
struct Vector3;

struct Static_Body
{
	b3BodyId id;
};

/// Creates a static body.
Static_Body create_static_body(const Physics& physics, const Vector3& position);
/// Destroys a static body.
void destroy_static_body(Static_Body& body);

struct Dynamic_Body
{
	b3BodyId id;
};

/// Creates a dynamic body.
Dynamic_Body create_dynamic_body(const Physics& physics, const Vector3& position);
/// Destroys a dynamic body.
void destroy_dynamic_body(Dynamic_Body& body);

/// Sets the transform of `body`.
void set_body_transform(const Static_Body& body, const Matrix4& world_matrix);
}  // namespace blk
