// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

// Descriptor set 0: a single set shared by every frame, written once at initialization.
// The bindings below are declared in the same order as `global_bindings` in `Lifetime/Descriptor.cpp`.

/// The transmittance LUT as a storage image, written once by `CS_Skybox_Transmittance.slang`.
/// The format should match the image created in `Renderer.cpp`.
[[vk::binding(0, 0)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> transmittance_storage_image;

/// The same image as `transmittance_storage_image`, sampled after it has been baked.
[[vk::binding(1, 0)]]
Sampler2D transmittance_lut;
