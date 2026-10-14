// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Pass/Line.slang"

[shader("fragment")]
float4
fs_main(VS_Line_Output input) : SV_Target
{
	return float4(input.color, 1.0);
}
