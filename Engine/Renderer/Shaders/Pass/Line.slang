// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

// Data passed from a line vertex shader to the fragment shader.
struct VS_Line_Output
{
	float4 position : SV_Position;
	float3 color;
};
