// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

// Descriptor set 0: a single set shared by every frame.
// The bindings below are declared in the same order as `global_bindings` in `Lifetime/Descriptor.cpp`.

/// `Terrain_Settings::heightmap_handle`, a single-channel `R16_UNORM` texture.
[[vk::binding(0, 0)]]
Sampler2D terrain_heightmap;
