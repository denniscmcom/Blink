// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

// Descriptor set 2: one set per material.
// The bindings below are declared in the same order as `material_bindings` in `Lifetime/Descriptor.cpp`.
//
// Binding 0 is the material uniform buffer. The host declares it in the set layout, but no shader reads it yet, so it
// has no declaration here.

[[vk::binding(1, 2)]]
Sampler2D albedo_map;

[[vk::binding(2, 2)]]
Sampler2D normal_map;

/// Occlusion, roughness and metallic packed into the red, green and blue channels.
[[vk::binding(3, 2)]]
Sampler2D orm_map;
