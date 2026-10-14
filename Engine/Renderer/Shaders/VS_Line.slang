// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Vertex_Input.slang"
#include "Pass/Line.slang"

[shader("vertex")]
VS_Line_Output
vs_main(VS_Vertex_Color input)
{
	VS_Line_Output output = {};
	output.position = mul(camera_ubo.projection, mul(camera_ubo.view, float4(input.position, 1.0)));
	output.color = input.color;

	return output;
}
