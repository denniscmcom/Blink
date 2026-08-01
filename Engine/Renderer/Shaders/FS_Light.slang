// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Shared.slang"

[[vk::binding(0, 1)]]
ConstantBuffer<Light_UBO> light_ubo;

[[vk::push_constant]]
ConstantBuffer<Light_Constants> light_constants;

[shader("fragment")]
float4
fs_main(VS_PNT_Output input)
	: SV_Target
{
	return float4(light_ubo.light_colors[light_constants.light_index], 1.0);
}
